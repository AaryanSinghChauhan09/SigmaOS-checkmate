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
  property real lift: 0
  property real presence: record.alive ? 1 : 0
  property bool ready: false
  property color bodyColor: design === 0 ? (record.white ? "#e6dfcb" : "#333b49")
                            : design === 1 ? (record.white ? "#ead6ad" : "#603a28")
                            : design === 2 ? (record.white ? "#e3d8b5" : "#a65e3b")
                            : (record.white ? "#ef9f9b" : "#8e6bab")
  property color trimColor: design === 0 ? (record.white ? "#7dc9c6" : "#e3ac5c")
                            : design === 1 ? "#d2a667" : design === 2 ? "#75854d" : "#e9c88c"
  x: record.square % 8 - 3.5
  z: 3.5 - Math.floor(record.square / 8)
  y: .12 + lift + (selected ? .1 : 0)
  scale: Qt.vector3d(presence, presence, presence)
  eulerRotation.y: record.kind === 2 ? (record.white ? 90 : -90) : (record.white ? 180 : 0)
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
    geometry: SculptGeometry { kind: piece.record.kind; design: piece.design; part: 0 }
    materials: PrincipledMaterial {
      baseColorMap: Texture { textureData: SurfaceTexture { style: piece.design } tilingModeVertical: Texture.Repeat }
      baseColor: piece.bodyColor
      metalness: piece.design === 0 ? .32 : 0
      roughness: piece.design === 3 ? .22 : piece.design === 1 ? .36 : .48
      specularAmount: .65
      clearcoatAmount: piece.design === 3 ? .8 : .18
      clearcoatRoughnessAmount: .2
      cullMode: Material.NoCulling
    }
  }
  Model {
    property int boardSquare: piece.record.square
    pickable: piece.record.alive
    geometry: SculptGeometry { kind: piece.record.kind; design: piece.design; part: 1 }
    materials: PrincipledMaterial {
      baseColor: piece.trimColor
      metalness: piece.design === 1 ? .6 : .25
      roughness: .3
      emissiveFactor: piece.design === 0 ? Qt.vector3d(.12,.12,.12) : Qt.vector3d(0,0,0)
      cullMode: Material.NoCulling
    }
  }
  Model {
    property int boardSquare: piece.record.square
    pickable: piece.record.alive
    geometry: SculptGeometry { kind: piece.record.kind; design: piece.design; part: 2 }
    materials: PrincipledMaterial { baseColor: "#18292c"; roughness: .25; cullMode: Material.NoCulling }
  }
}
