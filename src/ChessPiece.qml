import QtQuick
import QtQuick3D
import Checkmate 1.0

Node {
  id: piece
  objectName: "figure-" + pieceId
  required property int pieceId
  required property var record
  required property int design
  required property bool motion
  required property bool selected
  required property real seat
  property real lift: 0
  property real presence: record.alive ? 1 : 0
  property bool ready: false
  // Moss & Myth is one carved figure per piece, painted in its mesh rather than by part.
  property bool woodland: design === 2
  property color bodyColor: design === 0 ? (record.white ? "#e2d8c2" : "#343a44")
                            : design === 1 ? (record.white ? "#eed8aa" : "#59321b")
                            : design === 2 ? "#ffffff"
                            : (record.white ? "#f7949b" : "#9a5fa6")
  property color trimColor: design === 0 ? "#b99050" : design === 1 ? "#c89b51" : "#f2c653"
  property color secondaryColor: design === 0 ? (record.white ? "#52cdd0" : "#ffb84d")
                                 : design === 1 ? (record.white ? "#c8ac7e" : "#362115")
                                 : (record.white ? "#d25a82" : "#7b4a9c")
  // Sugar passes light through itself, so candy never falls into dark shadow: each sweet glows faintly in its own colour.
  property bool candy: design === 3
  property vector3d sugarGlow: !candy ? Qt.vector3d(0,0,0) : record.white ? Qt.vector3d(.24,.085,.095) : Qt.vector3d(.105,.05,.125)
  property vector3d secondGlow: design === 0 ? (record.white ? Qt.vector3d(.22,.95,1.05) : Qt.vector3d(1.25,.55,.08))
                                : !candy ? Qt.vector3d(0,0,0) : record.white ? Qt.vector3d(.15,.03,.06) : Qt.vector3d(.06,.025,.1)
  x: record.square % 8 - 3.5
  z: 3.5 - Math.floor(record.square / 8)
  y: .12 + lift + (selected ? .1 : 0)
  scale: Qt.vector3d(presence, presence, presence)
  // Every figure faces whoever is seated at the board. Knights turn in toward its middle: the foxes
  // a little, the horses far enough to show their profile.
  property real knightTurn: woodland ? 32 : 58
  eulerRotation.y: 180 + seat + (record.kind === 2 ? (pieceId % 2 ? knightTurn : -knightTurn) * Math.cos(seat * Math.PI / 180) : 0)
  Behavior on x { id: xBehavior; enabled: piece.ready && piece.motion; NumberAnimation { id: slideX; duration: 360; easing.type: Easing.InOutCubic } }
  Behavior on z { id: zBehavior; enabled: piece.ready && piece.motion; NumberAnimation { id: slideZ; duration: 360; easing.type: Easing.InOutCubic } }
  Behavior on y { id: yBehavior; enabled: piece.ready && piece.motion && !hop.running; NumberAnimation { id: selectionLift; duration: 180; easing.type: Easing.OutCubic } }
  Behavior on presence { id: presenceBehavior; enabled: piece.ready && piece.motion; NumberAnimation { id: captureShrink; duration: 220; easing.type: Easing.InCubic } }
  onMotionChanged: {
    if (!motion) {
      xBehavior.enabled = false; zBehavior.enabled = false
      yBehavior.enabled = false; presenceBehavior.enabled = false
      hop.stop(); lift = 0
      // Resolve moves queued before the first animation frame as well.
      x = Qt.binding(function() { return piece.record.square % 8 - 3.5 })
      z = Qt.binding(function() { return 3.5 - Math.floor(piece.record.square / 8) })
      y = Qt.binding(function() { return .12 + piece.lift + (piece.selected ? .1 : 0) })
      presence = Qt.binding(function() { return piece.record.alive ? 1 : 0 })
      xBehavior.enabled = Qt.binding(function() { return piece.ready && piece.motion })
      zBehavior.enabled = Qt.binding(function() { return piece.ready && piece.motion })
      yBehavior.enabled = Qt.binding(function() { return piece.ready && piece.motion && !hop.running })
      presenceBehavior.enabled = Qt.binding(function() { return piece.ready && piece.motion })
    }
  }
  Component.onCompleted: ready = true
  Connections {
    target: backend
    function onMoved(id, knight, capture) {
      if (id === piece.pieceId && piece.motion) {
        hop.height = knight ? .48 : .14
        hop.restart()
      }
    }
  }
  SequentialAnimation {
    id: hop
    objectName: "moveHop"
    property real height: .15
    NumberAnimation { target: piece; property: "lift"; to: hop.height; duration: 150; easing.type: Easing.OutQuad }
    NumberAnimation { target: piece; property: "lift"; to: 0; duration: 210; easing.type: Easing.InOutQuad }
  }
  Model {
    objectName: "piece-" + piece.pieceId
    property int boardSquare: piece.record.square
    pickable: piece.record.alive
    geometry: SculptGeometry { kind: piece.record.kind; design: piece.design; tone: piece.record.white ? 0 : 1; part: 0 }
    Texture { id: surface; textureData: SurfaceTexture { style: piece.design === 1 ? (piece.record.white ? 5 : 1) : piece.design } tilingModeVertical: Texture.Repeat }
    // Ceramic or gunmetal, polished wood, painted carving, boiled sugar.
    materials: PrincipledMaterial {
      baseColorMap: piece.woodland ? null : surface
      baseColor: piece.bodyColor
      vertexColorsEnabled: true
      metalness: piece.design === 0 ? (piece.record.white ? .2 : .5) : 0
      roughness: piece.candy ? .24 : piece.design === 1 ? .34 : piece.design === 0 ? (piece.record.white ? .3 : .36) : .7
      specularAmount: piece.woodland ? .3 : .5
      // Polished wood keeps its mirror for the lights it faces; the glancing sheen is held to the very edge.
      fresnelPower: piece.design === 1 ? 8 : 5
      // A full lacquer mirrors the dark of the room at every grazing edge, so the sweets wear only a light glaze.
      clearcoatAmount: piece.woodland ? 0 : piece.design === 0 ? .2 : .35
      clearcoatRoughnessAmount: piece.design === 1 ? .14 : piece.candy ? .08 : .2
      emissiveFactor: piece.sugarGlow
      cullMode: Material.NoCulling
    }
  }
  // The cast sets add metalwork, dark inlay and a second colour, each a solid of its own.
  Model {
    property int boardSquare: piece.record.square
    pickable: piece.record.alive
    visible: !piece.woodland
    geometry: SculptGeometry { kind: piece.record.kind; design: piece.design; part: 1 }
    materials: PrincipledMaterial {
      baseColor: piece.trimColor
      vertexColorsEnabled: true
      // Gold leaf on a sweet is warmer and softer than cast metal.
      metalness: piece.candy ? .6 : .9
      roughness: piece.candy ? .3 : .26
      emissiveFactor: piece.candy ? Qt.vector3d(.12,.08,.015) : Qt.vector3d(0,0,0)
      cullMode: Material.NoCulling
    }
  }
  Model {
    property int boardSquare: piece.record.square
    pickable: piece.record.alive
    visible: !piece.woodland
    geometry: SculptGeometry { kind: piece.record.kind; design: piece.design; part: 2 }
    materials: PrincipledMaterial {
      baseColor: piece.design === 3 ? (piece.record.white ? "#5a2238" : "#2c1636") : piece.design === 0 ? "#10151c" : "#1b1a20"
      vertexColorsEnabled: true
      roughness: piece.design === 0 ? .1 : .25
      clearcoatAmount: piece.design === 0 ? .6 : 0
      cullMode: Material.NoCulling
    }
  }
  Model {
    property int boardSquare: piece.record.square
    pickable: piece.record.alive
    visible: !piece.woodland
    geometry: SculptGeometry { kind: piece.record.kind; design: piece.design; part: 3 }
    materials: PrincipledMaterial {
      baseColor: piece.secondaryColor
      vertexColorsEnabled: true
      roughness: piece.candy ? .24 : .2
      clearcoatAmount: piece.candy ? .35 : 0
      clearcoatRoughnessAmount: piece.candy ? .08 : .2
      // Orbital lights burn in their own colour whatever the room is doing.
      emissiveFactor: piece.secondGlow
      cullMode: Material.NoCulling
    }
  }
}
