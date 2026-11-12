import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

ApplicationWindow {
  id: window
  width: 1280
  height: 850
  minimumWidth: 920
  minimumHeight: 650
  visible: true
  title: "Checkmate"
  color: theme.background
  Material.theme: theme.light ? Material.Light : Material.Dark
  Material.accent: theme.accent
  Material.background: theme.background
  Material.foreground: theme.foreground
  font.family: "sans-serif"
  font.pixelSize: 14
  property color line: Qt.alpha(theme.foreground,.13)
  property color muted: Qt.alpha(theme.foreground,.58)
  property var setNames: [qsTr("Orbital Orders"),qsTr("Midnight Atelier"),qsTr("Moss & Myth"),qsTr("Candy Kingdom")]
  property var setDescriptions: [qsTr("Across the stars"),qsTr("Wood, ivory & brass"),qsTr("A woodland tale"),qsTr("A little sweetness")]
  property var setColors: ["#d9ad69","#c99b6b","#93a775","#db9eb2"]
  property var difficultyNames: [qsTr("Casual"),qsTr("Thoughtful"),qsTr("Challenging")]
  property bool modalOpen: newGameDialog.opened || settingsDialog.opened || helpDialog.opened || promotionDialog.opened
  function statusText() {
    if (backend.outcome === "white-wins") return qsTr("White wins by checkmate")
    if (backend.outcome === "black-wins") return qsTr("Black wins by checkmate")
    if (backend.outcome === "stalemate") return qsTr("Draw by stalemate")
    if (backend.outcome === "material") return qsTr("Draw · insufficient material")
    if (backend.outcome === "fifty-moves") return qsTr("Draw · fifty-move rule")
    if (backend.outcome === "repetition") return qsTr("Draw · threefold repetition")
    if (backend.thinking) return qsTr("Considering the next move…")
    if (backend.checked) return backend.whiteTurn ? qsTr("White is in check") : qsTr("Black is in check")
    return backend.whiteTurn ? qsTr("White to move") : qsTr("Black to move")
  }
  component FlatButton: Button {
    id: control
    Material.roundedScale: theme.radius
    implicitHeight: 38
    background: Rectangle {
      radius: theme.radius
      color: control.down ? Qt.alpha(theme.accent,.22) : control.hovered ? Qt.alpha(theme.foreground,.09) : "transparent"
      border.color: window.line
    }
    contentItem: Text {
      text: control.text
      color: control.enabled ? theme.foreground : window.muted
      opacity: control.enabled ? 1 : .5
      font: control.font
      horizontalAlignment: Text.AlignHCenter
      verticalAlignment: Text.AlignVCenter
    }
  }
  Item {
    anchors.fill: parent
    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true
    ColumnLayout {
      anchors.fill: parent
      spacing: 0
      Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 70
        color: theme.background
        RowLayout {
          anchors { fill: parent; leftMargin: 26; rightMargin: 24 }
          spacing: 13
          Text { text: "♔"; color: window.setColors[backend.pieceSet]; font.pixelSize: 32 }
          Text { text: "Checkmate"; color: theme.foreground; font.family: "serif"; font.pixelSize: 29 }
          Rectangle { Layout.leftMargin: 12; width: 1; height: 22; color: window.line }
          Text { text: window.setNames[backend.pieceSet]; color: window.muted; font.pixelSize: 12; Layout.fillWidth: true }
          FlatButton { objectName: "newGameButton"; text: qsTr("+  New game"); onClicked: newGameDialog.open() }
          FlatButton { text: qsTr("Settings"); onClicked: settingsDialog.open() }
          FlatButton { text: "?"; implicitWidth: 38; Accessible.name: qsTr("Keyboard shortcuts"); onClicked: helpDialog.open() }
        }
      }
      Rectangle { Layout.fillWidth: true; height: 1; color: window.line }
      RowLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        spacing: 0
        BoardView { id: board; objectName: "board"; Layout.fillWidth: true; Layout.fillHeight: true }
        Rectangle { Layout.fillHeight: true; width: 1; color: window.line }
        Rectangle {
          Layout.preferredWidth: window.width < 1100 ? 264 : 298
          Layout.fillHeight: true
          color: Qt.alpha(theme.foreground,.018)
          ColumnLayout {
            anchors { fill: parent; margins: 22 }
            spacing: 18
            RowLayout {
              Layout.fillWidth: true
              Text { text: backend.computer ? qsTr("VS COMPUTER") : qsTr("TWO PLAYERS"); font.pixelSize: 10; font.letterSpacing: 2; color: window.muted; Layout.fillWidth: true }
              Text { text: "●"; color: window.setColors[backend.pieceSet]; font.pixelSize: 9 }
            }
            Text { text: backend.computer ? window.difficultyNames[backend.difficulty] : qsTr("One board. Two minds."); color: theme.foreground; font.family: "serif"; font.pixelSize: 23; Layout.fillWidth: true; wrapMode: Text.WordWrap }
            Repeater {
              model: [false,true]
              Rectangle {
                required property bool modelData
                Layout.fillWidth: true
                Layout.preferredHeight: 64
                color: backend.whiteTurn === modelData && !backend.finished ? Qt.alpha(theme.accent,.08) : "transparent"
                radius: theme.radius
                border.color: backend.whiteTurn === modelData && !backend.finished ? Qt.alpha(theme.accent,.45) : window.line
                RowLayout {
                  anchors { fill: parent; margins: 12 }
                  spacing: 12
                  Text { text: modelData ? "♙" : "♟"; font.pixelSize: 29; color: theme.foreground }
                  ColumnLayout {
                    spacing: 3
                    Text { text: backend.computer ? (backend.humanWhite === modelData ? qsTr("You") : qsTr("Computer")) : (modelData ? qsTr("Player 1") : qsTr("Player 2")); color: theme.foreground; font.pixelSize: 14 }
                    Text { text: modelData ? qsTr("White") : qsTr("Black"); color: window.muted; font.pixelSize: 11 }
                  }
                  Item { Layout.fillWidth: true }
                  Text { text: backend.whiteTurn === modelData && !backend.finished ? "●" : ""; color: theme.accent; font.pixelSize: 10 }
                }
              }
            }
            Rectangle { Layout.fillWidth: true; height: 1; color: window.line }
            Text { objectName: "status"; text: window.statusText(); color: backend.checked ? theme.colors.red : theme.foreground; font.pixelSize: 17; font.family: "serif"; wrapMode: Text.WordWrap; Layout.fillWidth: true }
            Text { text: qsTr("MOVE HISTORY"); color: window.muted; font.pixelSize: 10; font.letterSpacing: 1.8 }
            ListView {
              id: history
              objectName: "history"
              Layout.fillWidth: true
              Layout.fillHeight: true
              clip: true
              model: Math.ceil(backend.moves.length / 2)
              spacing: 3
              ScrollBar.vertical: ScrollBar { }
              onCountChanged: Qt.callLater(positionViewAtEnd)
              delegate: Rectangle {
                required property int index
                width: history.width
                height: 32
                color: index % 2 === 0 ? Qt.alpha(theme.foreground,.035) : "transparent"
                RowLayout {
                  anchors { fill: parent; leftMargin: 8; rightMargin: 8 }
                  Text { text: { translator.language; return (index + 1).toLocaleString(Qt.locale(),'f',0) + "." } color: window.muted; Layout.preferredWidth: 30; font.pixelSize: 12 }
                  Text { text: backend.moves[index * 2] || ""; color: theme.foreground; Layout.fillWidth: true; font.pixelSize: 13 }
                  Text { text: backend.moves[index * 2 + 1] || "·"; color: theme.foreground; Layout.preferredWidth: 66; font.pixelSize: 13 }
                }
              }
              Text { anchors { top: parent.top; left: parent.left } visible: history.count === 0; text: qsTr("Every great game starts\nwith a single move."); color: window.muted; font.pixelSize: 12; lineHeight: 1.6 }
            }
            FlatButton { objectName: "undoButton"; text: qsTr("↶  Undo move"); Layout.fillWidth: true; enabled: backend.canUndo; onClicked: backend.undo() }
            Text { text: qsTr("Saved automatically · No time limit"); font.pixelSize: 10; color: window.muted; Layout.alignment: Qt.AlignHCenter }
          }
        }
      }
      Rectangle { Layout.fillWidth: true; height: 1; color: window.line }
      Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 104
        color: theme.background
        RowLayout {
          anchors { fill: parent; leftMargin: 24; rightMargin: 24; topMargin: 14; bottomMargin: 14 }
          spacing: 12
          Repeater {
            model: 4
            Button {
              id: setButton
              required property int index
              Layout.fillWidth: true
              Layout.fillHeight: true
              objectName: "set-" + index
              Accessible.name: window.setNames[index]
              onClicked: backend.pieceSet = index
              background: Rectangle {
                radius: theme.radius
                color: backend.pieceSet === setButton.index ? Qt.alpha(window.setColors[setButton.index],.1) : setButton.hovered ? Qt.alpha(theme.foreground,.04) : "transparent"
                border.color: backend.pieceSet === setButton.index ? window.setColors[setButton.index] : window.line
              }
              contentItem: RowLayout {
                spacing: 12
                Text { text: ["◎","♞","❧","♛"][setButton.index]; color: window.setColors[setButton.index]; font.pixelSize: 30; Layout.leftMargin: 12 }
                ColumnLayout {
                  Layout.fillWidth: true
                  spacing: 5
                  Text { text: window.setNames[setButton.index]; color: theme.foreground; font.pixelSize: 12; font.bold: true }
                  Text { text: window.setDescriptions[setButton.index]; color: window.muted; font.pixelSize: 10; visible: window.width > 1050 }
                }
                Text { text: backend.pieceSet === setButton.index ? "✓" : ""; color: window.setColors[setButton.index]; Layout.rightMargin: 10 }
              }
            }
          }
        }
      }
    }
    Rectangle {
      visible: backend.error.length > 0
      anchors { left: parent.left; right: parent.right; bottom: parent.bottom; margins: 16 }
      height: errorRow.implicitHeight + 28
      color: theme.background
      border.color: theme.colors.red
      radius: theme.radius
      RowLayout {
        id: errorRow
        anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; margins: 14 }
        Text { text: backend.error; color: theme.foreground; wrapMode: Text.WordWrap; Layout.fillWidth: true }
        FlatButton { text: qsTr("Dismiss"); onClicked: backend.dismissError() }
      }
    }
  }
  Dialog {
    id: newGameDialog
    objectName: "newGameDialog"
    anchors.centerIn: parent
    width: 420
    modal: true
    focus: true
    title: qsTr("A new match")
    standardButtons: Dialog.Ok | Dialog.Cancel
    Material.roundedScale: theme.radius
    onOpened: {
      mode.currentIndex = backend.computer ? 0 : 1
      level.currentIndex = backend.difficulty
      side.currentIndex = backend.humanWhite ? 0 : 1
    }
    onAccepted: { backend.newGame(mode.currentIndex === 0,level.currentIndex,side.currentIndex === 0); board.resetCamera() }
    contentItem: ColumnLayout {
      LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
      LayoutMirroring.childrenInherit: true
      spacing: 16
      Label { text: backend.canUndo ? qsTr("Starting a new match replaces the saved game.") : qsTr("Choose your opponent and settle in."); wrapMode: Text.WordWrap; Layout.fillWidth: true }
      ComboBox { id: mode; objectName: "mode"; Layout.fillWidth: true; model: [qsTr("Play against the computer"),qsTr("Two players · same device")]; Material.roundedScale: theme.radius }
      ComboBox { id: level; Layout.fillWidth: true; model: window.difficultyNames; visible: mode.currentIndex === 0; Material.roundedScale: theme.radius }
      ComboBox { id: side; Layout.fillWidth: true; model: [qsTr("Play as White"),qsTr("Play as Black")]; visible: mode.currentIndex === 0; Material.roundedScale: theme.radius }
    }
  }
  Dialog {
    id: settingsDialog
    anchors.centerIn: parent
    width: 400
    modal: true
    focus: true
    title: qsTr("Make yourself comfortable")
    standardButtons: Dialog.Close
    Material.roundedScale: theme.radius
    contentItem: ColumnLayout {
      LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
      LayoutMirroring.childrenInherit: true
      CheckBox { text: qsTr("Reduced motion"); checked: backend.reducedMotion; onToggled: backend.reducedMotion = checked }
      CheckBox { text: qsTr("Turn the board for each player"); checked: backend.autoRotate; onToggled: backend.autoRotate = checked }
      Label { text: qsTr("Automatic rotation applies to two-player games."); color: window.muted; wrapMode: Text.WordWrap; Layout.fillWidth: true }
      FlatButton { text: qsTr("Reset camera"); Layout.fillWidth: true; onClicked: board.resetCamera() }
    }
  }
  Dialog {
    id: helpDialog
    objectName: "help"
    anchors.centerIn: parent
    width: 430
    modal: true
    focus: true
    title: qsTr("At your fingertips")
    standardButtons: Dialog.Close
    Material.roundedScale: theme.radius
    contentItem: Label {
      LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
      text: qsTr("Click a piece, then a highlighted square.\n\nArrow keys  ·  Navigate the board\nSpace / Enter  ·  Select or move\nEscape  ·  Cancel selection\nCtrl+Z  ·  Undo\nN  ·  New game\nF  ·  Flip board\nR  ·  Reset camera\n?  ·  Keyboard shortcuts\nQ  ·  Quit (your game is saved)\n\nRight-drag to orbit. Scroll to zoom.")
      wrapMode: Text.WordWrap
      lineHeight: 1.4
    }
  }
  Dialog {
    id: promotionDialog
    anchors.centerIn: parent
    width: 420
    modal: true
    focus: true
    title: qsTr("Choose your promotion")
    closePolicy: Popup.CloseOnEscape
    Material.roundedScale: theme.radius
    onRejected: backend.cancelSelection()
    contentItem: RowLayout {
      LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
      LayoutMirroring.childrenInherit: true
      Repeater {
        model: [5,4,3,2]
        FlatButton {
          required property int modelData
          text: ["","","♘","♗","♖","♕"][modelData]
          font.pixelSize: 32
          Layout.fillWidth: true
          Accessible.name: modelData === 5 ? qsTr("Queen") : modelData === 4 ? qsTr("Rook") : modelData === 3 ? qsTr("Bishop") : qsTr("Knight")
          onClicked: { backend.promote(modelData); promotionDialog.close() }
        }
      }
    }
  }
  Connections {
    target: backend
    function onChanged() {
      if (backend.promotionPending && !promotionDialog.opened) promotionDialog.open()
      else if (!backend.promotionPending && promotionDialog.opened) promotionDialog.close()
    }
  }
  Shortcut { sequence: "?"; enabled: !window.modalOpen; onActivated: helpDialog.open() }
  Shortcut { sequence: "Q"; enabled: !window.modalOpen; onActivated: window.close() }
  Shortcut { sequence: "N"; enabled: !window.modalOpen; onActivated: newGameDialog.open() }
  Shortcut { sequence: StandardKey.Undo; enabled: !window.modalOpen; onActivated: backend.undo() }
  Shortcut { sequence: "F"; enabled: !window.modalOpen; onActivated: board.azimuth += 180 }
  Shortcut { sequence: "R"; enabled: !window.modalOpen; onActivated: board.resetCamera() }
  Shortcut { sequence: "Escape"; enabled: !window.modalOpen; onActivated: backend.cancelSelection() }
  Shortcut { sequences: ["Space","Return"]; enabled: !window.modalOpen; onActivated: board.keyboardSelect() }
  Shortcut { sequence: "Left"; enabled: !window.modalOpen; onActivated: board.navigate(-1,0) }
  Shortcut { sequence: "Right"; enabled: !window.modalOpen; onActivated: board.navigate(1,0) }
  Shortcut { sequence: "Up"; enabled: !window.modalOpen; onActivated: board.navigate(0,1) }
  Shortcut { sequence: "Down"; enabled: !window.modalOpen; onActivated: board.navigate(0,-1) }
}
