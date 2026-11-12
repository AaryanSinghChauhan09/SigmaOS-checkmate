#include "chess.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <sstream>
#include <limits>
#include <stdexcept>

namespace Chess {
namespace {
const char *letters = " PNBRQK";
const int values[] = {0, 100, 320, 335, 500, 900, 0};
bool inside(int x, int y) { return x >= 0 && x < 8 && y >= 0 && y < 8; }
}
std::string squareName(int s) { return {char('a' + s % 8), char('1' + s / 8)}; }
int square(const std::string &s) {
  if (s.size() != 2 || s[0] < 'a' || s[0] > 'h' || s[1] < '1' || s[1] > '8') return -1;
  return (s[1] - '1') * 8 + s[0] - 'a';
}
Position Position::initial() { return fromFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"); }
Position Position::fromFen(const std::string &fen) {
  Position p;
  std::istringstream input(fen);
  std::string rows, turn, rights, enPassant;
  if (!(input >> rows >> turn >> rights >> enPassant >> p.halfmove >> p.fullmove)) throw std::invalid_argument("Invalid FEN");
  int rank = 7, file = 0, id = 0;
  for (char c : rows) {
    if (c == '/') { if (file != 8 || rank == 0) throw std::invalid_argument("Invalid FEN board"); --rank; file = 0; }
    else if (c >= '1' && c <= '8') file += c - '0';
    else {
      const char upper = char(std::toupper(static_cast<unsigned char>(c)));
      const std::string kinds = "PNBRQK";
      auto found = kinds.find(upper);
      if (found == std::string::npos || file >= 8 || id >= 32) throw std::invalid_argument("Invalid FEN piece");
      p.board[rank * 8 + file++] = {int(found + 1), c == upper, id++};
    }
    if (file > 8) throw std::invalid_argument("Invalid FEN rank");
  }
  if (rank != 0 || file != 8 || (turn != "w" && turn != "b") || p.halfmove < 0 || p.fullmove < 1) throw std::invalid_argument("Invalid FEN fields");
  p.white = turn == "w";
  p.castle = 0;
  if (rights != "-") for (char c : rights) {
    const auto found = std::string("KQkq").find(c);
    if (found == std::string::npos) throw std::invalid_argument("Invalid castling rights");
    p.castle |= 1 << found;
  }
  p.ep = enPassant == "-" ? -1 : square(enPassant);
  if (enPassant != "-" && (p.ep < 0 || p.ep / 8 != (p.white ? 5 : 2))) throw std::invalid_argument("Invalid en passant square");
  for (bool side : {false, true}) {
    int kings = 0;
    for (int s = 0; s < 64; ++s) {
      const auto piece = p.board[s];
      kings += piece.kind == King && piece.white == side;
      if (piece.kind == Pawn && (s / 8 == 0 || s / 8 == 7)) throw std::invalid_argument("Pawn on last rank");
    }
    if (kings != 1) throw std::invalid_argument("Position needs two kings");
  }
  return p;
}
std::string Position::fen() const {
  std::string out;
  for (int rank = 7; rank >= 0; --rank) {
    int empty = 0;
    for (int file = 0; file < 8; ++file) {
      auto p = board[rank * 8 + file];
      if (!p.kind) ++empty;
      else {
        if (empty) { out += char('0' + empty); empty = 0; }
        out += p.white ? letters[p.kind] : char(std::tolower(letters[p.kind]));
      }
    }
    if (empty) out += char('0' + empty);
    if (rank) out += '/';
  }
  out += white ? " w " : " b ";
  if (!castle) out += '-';
  for (int i = 0; i < 4; ++i) if (castle & (1 << i)) out += "KQkq"[i];
  out += " " + (ep < 0 ? std::string("-") : squareName(ep));
  return out + " " + std::to_string(halfmove) + " " + std::to_string(fullmove);
}
std::string Position::key() const {
  Position normalized = *this;
  if (ep >= 0) {
    bool possible = false;
    for (auto move : legal()) if (move.to == ep && board[move.from].kind == Pawn && move.from % 8 != ep % 8) possible = true;
    if (!possible) normalized.ep = -1;
  }
  std::string value = normalized.fen();
  return value.substr(0, value.rfind(' ', value.rfind(' ') - 1));
}
bool Position::attacked(int s, bool side) const {
  const int x = s % 8, y = s / 8;
  for (int dx : {-1, 1}) {
    int py = y + (side ? -1 : 1);
    if (inside(x + dx, py)) {
      auto p = board[py * 8 + x + dx];
      if (p.kind == Pawn && p.white == side) return true;
    }
  }
  for (int dx = -2; dx <= 2; ++dx) for (int dy = -2; dy <= 2; ++dy) {
    if (std::abs(dx * dy) != 2 || !inside(x + dx, y + dy)) continue;
    auto p = board[(y + dy) * 8 + x + dx];
    if (p.kind == Knight && p.white == side) return true;
  }
  for (int dx = -1; dx <= 1; ++dx) for (int dy = -1; dy <= 1; ++dy) {
    if (!dx && !dy) continue;
    for (int d = 1; inside(x + dx * d, y + dy * d); ++d) {
      auto p = board[(y + dy * d) * 8 + x + dx * d];
      if (!p.kind) continue;
      if (p.white == side && (p.kind == Queen || (p.kind == King && d == 1)
          || (p.kind == Bishop && dx && dy) || (p.kind == Rook && (!dx || !dy)))) return true;
      break;
    }
  }
  return false;
}
bool Position::check(bool side) const {
  for (int i = 0; i < 64; ++i) if (board[i].kind == King && board[i].white == side) return attacked(i, !side);
  return true;
}
bool Position::insufficient() const {
  int minors = 0, bishopsColor = -1;
  bool bishopsOnly = true;
  for (int s = 0; s < 64; ++s) {
    int k = board[s].kind;
    if (k == Pawn || k == Rook || k == Queen) return false;
    if (k == Knight) { ++minors; bishopsOnly = false; }
    if (k == Bishop) {
      ++minors;
      int color = (s % 8 + s / 8) % 2;
      if (bishopsColor >= 0 && bishopsColor != color) bishopsOnly = false;
      bishopsColor = color;
    }
  }
  return minors <= 1 || bishopsOnly;
}
Position Position::after(Move move) const {
  Position next = *this;
  Piece piece = board[move.from];
  const bool capture = board[move.to].kind || (piece.kind == Pawn && move.to == ep);
  next.board[move.to] = piece;
  next.board[move.from] = {};
  if (piece.kind == Pawn && move.to == ep && move.from % 8 != move.to % 8) next.board[move.to + (piece.white ? -8 : 8)] = {};
  if (piece.kind == King && std::abs(move.to - move.from) == 2) {
    const int from = move.to > move.from ? move.from + 3 : move.from - 4;
    const int to = move.to > move.from ? move.from + 1 : move.from - 1;
    next.board[to] = board[from]; next.board[from] = {};
  }
  if (move.promotion) next.board[move.to].kind = move.promotion;
  if (piece.kind == King) next.castle &= piece.white ? ~3 : ~12;
  for (auto entry : {std::pair<int,int>{0,2}, {7,1}, {56,8}, {63,4}})
    if (move.from == entry.first || move.to == entry.first) next.castle &= ~entry.second;
  next.ep = piece.kind == Pawn && std::abs(move.to - move.from) == 16 ? (move.to + move.from) / 2 : -1;
  next.halfmove = piece.kind == Pawn || capture ? 0 : halfmove < std::numeric_limits<int>::max() ? halfmove + 1 : halfmove;
  next.fullmove += !white && fullmove < std::numeric_limits<int>::max();
  next.white = !white;
  return next;
}
std::vector<Move> Position::legal() const {
  std::vector<Move> candidates, result;
  auto add = [&](int from, int to) {
    if (board[to].kind == King) return;
    if (board[from].kind == Pawn && (to / 8 == 0 || to / 8 == 7))
      for (int k : {Queen, Rook, Bishop, Knight}) candidates.push_back({from,to,k});
    else candidates.push_back({from,to});
  };
  for (int from = 0; from < 64; ++from) {
    const auto p = board[from];
    if (!p.kind || p.white != white) continue;
    const int x = from % 8, y = from / 8;
    if (p.kind == Pawn) {
      const int step = white ? 1 : -1, ny = y + step;
      if (inside(x, ny) && !board[ny * 8 + x].kind) {
        add(from,ny * 8 + x);
        if (y == (white ? 1 : 6) && !board[(y + 2 * step) * 8 + x].kind) add(from,(y + 2 * step) * 8 + x);
      }
      for (int dx : {-1,1}) if (inside(x + dx,ny)) {
        int to = ny * 8 + x + dx;
        auto target = board[to];
        bool enPassant = to == ep && !target.kind && board[to - step * 8].kind == Pawn && board[to - step * 8].white != white;
        if ((target.kind && target.white != white) || enPassant) add(from,to);
      }
    } else if (p.kind == Knight) {
      for (int dx = -2; dx <= 2; ++dx) for (int dy = -2; dy <= 2; ++dy)
        if (std::abs(dx * dy) == 2 && inside(x + dx,y + dy)) {
          int to = (y + dy) * 8 + x + dx;
          if (!board[to].kind || board[to].white != white) add(from,to);
        }
    } else {
      for (int dx = -1; dx <= 1; ++dx) for (int dy = -1; dy <= 1; ++dy) {
        if ((!dx && !dy) || (p.kind == Bishop && (!dx || !dy)) || (p.kind == Rook && dx && dy)) continue;
        for (int d = 1; inside(x + dx * d,y + dy * d); ++d) {
          int to = (y + dy * d) * 8 + x + dx * d;
          if (board[to].kind && board[to].white == white) break;
          add(from,to);
          if (board[to].kind || p.kind == King) break;
        }
      }
      const int home = white ? 4 : 60;
      if (p.kind == King && from == home && !check(white)) {
        for (bool kingSide : {true,false}) {
          const int right = (white ? 1 : 4) * (kingSide ? 1 : 2);
          const int dir = kingSide ? 1 : -1, rook = home + (kingSide ? 3 : -4);
          if (!(castle & right) || board[rook].kind != Rook || board[rook].white != white) continue;
          bool clear = true;
          for (int s = home + dir; s != rook; s += dir) if (board[s].kind) clear = false;
          if (clear && !attacked(home + dir,!white) && !attacked(home + 2 * dir,!white)) add(home,home + 2 * dir);
        }
      }
    }
  }
  for (auto move : candidates) if (!after(move).check(white)) result.push_back(move);
  return result;
}
std::string Position::san(Move move) const {
  auto piece = board[move.from];
  std::string text;
  if (piece.kind == King && std::abs(move.to - move.from) == 2) text = move.to > move.from ? "O-O" : "O-O-O";
  else {
    bool capture = board[move.to].kind || (piece.kind == Pawn && move.to == ep);
    if (piece.kind != Pawn) {
      text += letters[piece.kind];
      bool ambiguous = false, sameFile = false, sameRank = false;
      for (auto m : legal()) if (m.to == move.to && m.from != move.from && board[m.from].kind == piece.kind) {
        ambiguous = true; sameFile |= m.from % 8 == move.from % 8; sameRank |= m.from / 8 == move.from / 8;
      }
      if (ambiguous) {
        if (!sameFile) text += char('a' + move.from % 8);
        else if (!sameRank) text += char('1' + move.from / 8);
        else text += squareName(move.from);
      }
    } else if (capture) text += char('a' + move.from % 8);
    if (capture) text += 'x';
    text += squareName(move.to);
    if (move.promotion) { text += '='; text += letters[move.promotion]; }
  }
  auto next = after(move);
  if (next.check(next.white)) text += next.legal().empty() ? '#' : '+';
  return text;
}
uint64_t perft(const Position &p, int depth) {
  if (!depth) return 1;
  uint64_t nodes = 0;
  for (auto move : p.legal()) nodes += perft(p.after(move),depth - 1);
  return nodes;
}
namespace {
struct Search {
  std::shared_ptr<std::atomic_bool> cancel;
  std::chrono::steady_clock::time_point deadline;
  std::vector<std::string> path;
  bool stopped = false;
  int evaluate(const Position &p) {
    int score = 0;
    for (int s = 0; s < 64; ++s) {
      auto piece = p.board[s];
      if (!piece.kind) continue;
      int rank = piece.white ? s / 8 : 7 - s / 8;
      int center = 7 - std::abs(2 * (s % 8) - 7) / 2 - std::abs(2 * rank - 7) / 2;
      int bonus = piece.kind == Pawn ? rank * 7 + center * 2 : piece.kind == Knight || piece.kind == Bishop ? center * 10 : 0;
      if (piece.kind == King && rank < 2 && (s % 8 == 6 || s % 8 == 2)) bonus += 25;
      score += (piece.white ? 1 : -1) * (values[piece.kind] + bonus);
    }
    return p.white ? score : -score;
  }
  int run(const Position &p, int depth, int alpha, int beta, int ply) {
    if (cancel->load() || std::chrono::steady_clock::now() >= deadline) { stopped = true; return 0; }
    auto moves = p.legal();
    if (moves.empty()) return p.check(p.white) ? -100000 + ply : 0;
    const auto key = p.key();
    if (p.halfmove >= 100 || p.insufficient() || std::count(path.begin(),path.end(),key) >= 2) return 0;
    if (depth <= 0 && ply >= 8) return evaluate(p);
    const bool check = p.check(p.white);
    if (depth <= 0 && !check) {
      int stand = evaluate(p);
      if (stand >= beta) return beta;
      alpha = std::max(alpha, stand);
      moves.erase(std::remove_if(moves.begin(),moves.end(),[&](Move m) {
        return !p.board[m.to].kind && !m.promotion && !(p.board[m.from].kind == Pawn && m.to == p.ep);
      }),moves.end());
    }
    std::stable_sort(moves.begin(),moves.end(),[&](Move a, Move b) {
      auto score = [&](Move m) { return values[p.board[m.to].kind] * 10 - values[p.board[m.from].kind] + values[m.promotion]; };
      return score(a) > score(b);
    });
    path.push_back(key);
    for (auto move : moves) {
      int score = -run(p.after(move),depth - 1,-beta,-alpha,ply + 1);
      if (stopped) break;
      if (score >= beta) { path.pop_back(); return beta; }
      alpha = std::max(alpha,score);
    }
    path.pop_back();
    return alpha;
  }
};
}
Move bestMove(const Position &p, int level, const std::shared_ptr<std::atomic_bool> &cancel, const std::vector<std::string> &history) {
  auto moves = p.legal();
  if (moves.empty()) return {};
  Search search{cancel, std::chrono::steady_clock::now() + std::chrono::milliseconds(level == 0 ? 180 : level == 1 ? 650 : 1600), history};
  Move best = moves.front();
  for (int depth = 1; depth <= (level == 0 ? 1 : level == 1 ? 3 : 5); ++depth) {
    std::stable_sort(moves.begin(),moves.end(),[&](Move a, Move b) { return (a == best) > (b == best); });
    int alpha = -1000000;
    Move iteration = best;
    for (auto move : moves) {
      int score = -search.run(p.after(move),depth - 1,-1000000,-alpha,1);
      if (search.stopped) break;
      if (score > alpha) { alpha = score; iteration = move; }
    }
    if (search.stopped) break;
    best = iteration;
  }
  return best;
}
}
