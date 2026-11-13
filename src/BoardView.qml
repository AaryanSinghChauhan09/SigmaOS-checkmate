import QtQuick
import QtQuick3D
import Checkmate 1.0

Item {
  id: board
  property int design: backend.pieceSet
  property bool motion: !backend.reducedMotion && Window.window && Window.window.active
  property real manualAzimuth: 0
  property real azimuth: (backend.computer ? (backend.humanWhite ? 0 : 180) : (backend.autoRotate && !backend.whiteTurn ? 180 : 0)) + manualAzimuth
  property real elevation: -44
  property real distance: 13.2
  property int cursorSquare: 12
  property bool keyboardMode: false
  property real checkGlow: .22
  property var lightSquares: ["#7a7c7e", "#e2caa1", "#c7c0a0", "#f6d6bc"]
  property var darkSquares: ["#252c33", "#54311e", "#65704c", "#99708e"]
  property var frames: ["#28343f", "#432b22", "#5b4934", "#e8c6be"]
  property var accents: ["#dcad62", "#cba265", "#9baa6b", "#dca6ab"]
  property bool sceneReady: false
  Component.onCompleted: Qt.callLater(function() { sceneReady = true })
  clip: true
  focus: true
  function resetCamera() {
    manualAzimuth = 0
    elevation = -44
    distance = 13.2
  }
  function keyboardSelect() { keyboardMode = true; backend.selectSquare(cursorSquare) }
  function navigate(dx, dy) {
    keyboardMode = true
    let angle = Math.round(azimuth / 90) * Math.PI / 2
    let fileStep = Math.round(dx * Math.cos(angle) - dy * Math.sin(angle))
    let rankStep = Math.round(dx * Math.sin(angle) + dy * Math.cos(angle))
    let x = Math.max(0, Math.min(7, cursorSquare % 8 + fileStep))
    let y = Math.max(0, Math.min(7, Math.floor(cursorSquare / 8) + rankStep))
    cursorSquare = y * 8 + x
  }
  Behavior on azimuth { id: rotationBehavior; enabled: board.motion && !orbit.dragging; NumberAnimation { id: cameraTurn; duration: 650; easing.type: Easing.InOutCubic } }
  onMotionChanged: {
    if (!motion) {
      rotationBehavior.enabled = false
      checkPulse.stop(); checkGlow = .22
      azimuth = Qt.binding(function() {
        return (backend.computer ? (backend.humanWhite ? 0 : 180) : (backend.autoRotate && !backend.whiteTurn ? 180 : 0)) + board.manualAzimuth
      })
      rotationBehavior.enabled = Qt.binding(function() { return board.motion && !orbit.dragging })
    }
  }
  Connections {
    target: backend
    function onMoved() { if (backend.checked && board.motion) checkPulse.restart() }
  }
  SequentialAnimation {
    id: checkPulse
    NumberAnimation { target: board; property: "checkGlow"; to: .7; duration: 220 }
    NumberAnimation { target: board; property: "checkGlow"; to: .22; duration: 500 }
  }
  Rectangle {
    anchors.fill: parent
    gradient: Gradient {
      GradientStop { position: 0; color: ["#0b1726","#49392e","#353b27","#e8c8c5"][board.design] }
      GradientStop { position: 1; color: ["#080e19","#1c1716","#1f261a","#ad8b9e"][board.design] }
    }
  }
  Image {
    anchors.fill: parent
    source: board.design === 0 ? "qrc:/scenes/orbital.png" : board.design === 2 ? "qrc:/scenes/woodland.png" : ""
    fillMode: Image.PreserveAspectCrop
    opacity: board.design === 0 ? .85 : .7
    visible: source.toString().length > 0
  }
  View3D {
    id: scene
    objectName: "scene"
    anchors.fill: parent
    environment: SceneEnvironment {
      backgroundMode: SceneEnvironment.Transparent
      lightProbe: Texture { textureData: SurfaceTexture { style: 4 } }
      probeExposure: .75
      antialiasingMode: SceneEnvironment.MSAA
      antialiasingQuality: SceneEnvironment.High
      tonemapMode: SceneEnvironment.TonemapModeAces
      aoEnabled: true
      aoStrength: 35
      aoDistance: .8
      aoSoftness: 50
    }
    Texture { id: surfaceMap; textureData: SurfaceTexture { style: board.design } tilingModeHorizontal: Texture.Repeat; tilingModeVertical: Texture.Repeat }
    Node {
      eulerRotation: Qt.vector3d(board.elevation,board.azimuth,0)
      PerspectiveCamera { id: camera; z: board.distance; clipNear: .1; clipFar: 150; fieldOfView: 43 }
    }
    camera: camera
    DirectionalLight {
      eulerRotation: Qt.vector3d(-45,-35,0)
      color: "#ffebd2"
      brightness: 1.05
      castsShadow: true
      shadowMapQuality: Light.ShadowMapQualityHigh
      shadowFactor: 80
      shadowBias: .015
      shadowMapFar: 40
    }
    DirectionalLight { eulerRotation: Qt.vector3d(-30,140,0); color: "#c2ddf4"; brightness: .35 }
    DirectionalLight { eulerRotation: Qt.vector3d(-70,0,0); color: "#ffffff"; brightness: .18 }
    Model {
      source: "#Cube"
      y: -.27
      scale: Qt.vector3d(.091,.005,.091)
      materials: PrincipledMaterial { baseColor: board.frames[board.design]; baseColorMap: surfaceMap; roughness: .38; metalness: board.design === 0 ? .4 : .05 }
    }
    Model {
      source: "#Cube"
      y: -.045
      scale: Qt.vector3d(.087,.0007,.087)
      materials: PrincipledMaterial { baseColor: board.accents[board.design]; roughness: .32; metalness: .5 }
    }
    Model {
      source: "#Cube"
      y: .001
      scale: Qt.vector3d(.084,.0005,.084)
      materials: PrincipledMaterial { baseColor: board.frames[board.design]; baseColorMap: surfaceMap; roughness: .4 }
    }
    Repeater3D {
      model: 64
      Model {
        id: tile
        required property int index
        property int boardSquare: index
        property bool target: backend.targets.indexOf(index) !== -1
        property bool marked: backend.selected === index || (board.keyboardMode && board.cursorSquare === index)
        source: "#Cube"
        objectName: "square-" + index
        pickable: true
        x: index % 8 - 3.5
        z: 3.5 - Math.floor(index / 8)
        y: .04
        scale: Qt.vector3d(.00996,.001,.00996)
        materials: PrincipledMaterial {
          baseColorMap: Texture { textureData: SurfaceTexture { style: board.design === 1 && (Math.floor(tile.index / 8) + tile.index % 8) % 2 === 1 ? 5 : board.design } }
          baseColor: tile.marked ? board.accents[board.design] : (Math.floor(index / 8) + index % 8) % 2 === 1 ? board.lightSquares[board.design] : board.darkSquares[board.design]
          roughness: board.design === 3 ? .3 : .55
          metalness: board.design === 0 ? .15 : 0
        }
        Model {
          source: "#Cylinder"
          visible: parent.target
          y: 56
          scale: Qt.vector3d(.23,1.1,.23)
          materials: PrincipledMaterial { baseColor: board.accents[board.design]; opacity: .8; lighting: PrincipledMaterial.NoLighting }
        }
      }
    }
    Repeater3D {
      model: 32
      ChessPiece {
        required property int index
        pieceId: index
        record: backend.pieces[index]
        design: board.design
        motion: board.motion
        selected: backend.selected === record.square && record.alive
      }
    }
    Repeater3D {
      model: [backend.lastFrom,backend.lastTo]
      Model {
        required property int modelData
        source: "#Cube"
        visible: modelData >= 0
        x: modelData % 8 - 3.5
        z: 3.5 - Math.floor(modelData / 8)
        y: .095
        scale: Qt.vector3d(.0095,.00005,.0095)
        materials: PrincipledMaterial { baseColor: board.accents[board.design]; opacity: .23; lighting: PrincipledMaterial.NoLighting }
      }
    }
    Repeater3D {
      model: 32
      Model {
        required property int index
        property var record: backend.pieces[index]
        source: "#Cylinder"
        visible: backend.checked && record.alive && record.kind === 6 && record.white === backend.whiteTurn
        x: record.square % 8 - 3.5
        z: 3.5 - Math.floor(record.square / 8)
        y: .10
        scale: Qt.vector3d(.009,.00005,.009)
        materials: PrincipledMaterial { baseColor: "#ed7369"; opacity: board.checkGlow; lighting: PrincipledMaterial.NoLighting }
      }
    }
  }
  Repeater {
    model: 16
    Text {
      required property int index
      property vector3d projected: {
        board.azimuth; board.elevation; board.distance; board.width; board.height
        if (!board.sceneReady) return Qt.vector3d(-100,-100,0)
        return scene.mapFrom3DScene(index < 8 ? Qt.vector3d(index - 3.5,.14,4.25) : Qt.vector3d(-4.25,.14,3.5 - (index - 8)))
      }
      x: projected.x - width / 2
      y: projected.y - height / 2
      text: index < 8 ? "abcdefgh"[index] : index - 7
      color: board.design === 3 ? "#5d3c58" : "#f2e6c8"
      font.pixelSize: 11
      font.bold: true
      opacity: .8
    }
  }
  MouseArea {
    id: orbit
    anchors.fill: parent
    acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
    property real previousX: 0
    property real previousY: 0
    property bool dragging: false
    onPressed: mouse => {
      previousX = mouse.x; previousY = mouse.y
      dragging = mouse.button !== Qt.LeftButton
      board.forceActiveFocus()
    }
    onPositionChanged: mouse => {
      if (dragging) {
        board.manualAzimuth -= (mouse.x - previousX) * .4
        board.elevation = Math.max(-80, Math.min(-25, board.elevation - (mouse.y - previousY) * .25))
        previousX = mouse.x; previousY = mouse.y
      }
    }
    onReleased: dragging = false
    onCanceled: dragging = false
    onClicked: mouse => {
      if (mouse.button !== Qt.LeftButton) return
      board.keyboardMode = false
      let hit = scene.pick(mouse.x, mouse.y)
      if (hit.objectHit && hit.objectHit.boardSquare !== undefined) backend.selectSquare(hit.objectHit.boardSquare)
      else backend.cancelSelection()
    }
    onWheel: wheel => { board.distance = Math.max(10.5, Math.min(20, board.distance - wheel.angleDelta.y / 600)) }
  }
  Row {
    anchors { bottom: parent.bottom; horizontalCenter: parent.horizontalCenter; bottomMargin: 20 }
    spacing: 24
    Text { text: qsTr("Right-drag to orbit"); color: board.design === 3 ? "#30232c" : "#eee3cc"; opacity: .65; font.pixelSize: 11 }
    Text { text: qsTr("Scroll to zoom"); color: board.design === 3 ? "#30232c" : "#eee3cc"; opacity: .65; font.pixelSize: 11 }
    Text { text: qsTr("F · Flip board"); color: board.design === 3 ? "#30232c" : "#eee3cc"; opacity: .65; font.pixelSize: 11 }
  }
}
