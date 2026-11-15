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
  property color bodyColor: design === 0 ? (record.white ? "#e2d8c2" : "#242a32")
                            : design === 1 ? (record.white ? "#eed8aa" : "#59321b")
                            : design === 2 ? "#ffffff"
                            : (record.white ? "#f38289" : "#935699")
  property color trimColor: design === 0 ? "#b99050" : design === 1 ? "#c89b51" : "#eabe58"
  property color secondaryColor: design === 0 ? (record.white ? "#52cdd0" : "#ffb84d")
                                 : design === 1 ? (record.white ? "#c8ac7e" : "#362115")
                                 : (record.white ? "#b94f72" : "#693b85")
  x: record.square % 8 - 3.5
  z: 3.5 - Math.floor(record.square / 8)
  y: .12 + lift + (selected ? .1 : 0)
  scale: Qt.vector3d(presence, presence, presence)
  // The woodland folk face whoever is seated at the board, the foxes turned in toward its middle.
  eulerRotation.y: woodland ? 180 + seat + (record.kind === 2 ? (pieceId % 2 ? 32 : -32) * Math.cos(seat * Math.PI / 180) : 0)
                   : record.kind === 2 ? (record.white ? 90 : -90) : (record.white ? 180 : 0)
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
    materials: PrincipledMaterial {
      baseColorMap: piece.woodland ? null : surface
      baseColor: piece.bodyColor
      vertexColorsEnabled: true
      metalness: piece.design === 0 ? .4 : 0
      roughness: piece.design === 3 ? .16 : piece.design === 1 ? .30 : piece.design === 0 ? .28 : .7
      specularAmount: piece.woodland ? .3 : .5
      clearcoatAmount: piece.design === 3 ? .9 : piece.woodland ? 0 : .12
      clearcoatRoughnessAmount: .2
      cullMode: Material.NoCulling
    }
  }
  Model {
    property int boardSquare: piece.record.square
    pickable: piece.record.alive
    visible: !piece.woodland
    geometry: SculptGeometry { kind: piece.record.kind; design: piece.design; part: 1 }
    materials: PrincipledMaterial {
      baseColor: piece.trimColor
      metalness: .8
      roughness: .25
      cullMode: Material.NoCulling
    }
  }
  Model {
    property int boardSquare: piece.record.square
    pickable: piece.record.alive
    visible: !piece.woodland
    geometry: SculptGeometry { kind: piece.record.kind; design: piece.design; part: 2 }
    materials: PrincipledMaterial { baseColor: "#1b1a20"; roughness: .25; cullMode: Material.NoCulling }
  }
  Repeater3D {
    model: 3
    Model {
      required property int index
      property int boardSquare: piece.record.square
      pickable: piece.record.alive
      visible: !piece.woodland
      geometry: SculptGeometry { kind: piece.record.kind; design: piece.design; part: 3 + index }
      materials: PrincipledMaterial {
        baseColor: index === 0 ? piece.secondaryColor : index === 1 ? (piece.record.kind === 5 ? "#f5e2bc" : "#ac793e") : (piece.record.white ? "#efd5ac" : "#dbad79")
        metalness: piece.design === 0 ? .25 : 0
        roughness: piece.design === 0 ? .18 : piece.design === 3 ? .20 : .55
        clearcoatAmount: piece.design === 3 ? .7 : 0
        emissiveFactor: piece.design === 0 ? (piece.record.white ? Qt.vector3d(.15,.6,.65) : Qt.vector3d(.75,.3,.04)) : Qt.vector3d(0,0,0)
        cullMode: Material.NoCulling
      }
    }
  }
}
