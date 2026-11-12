import QtQuick
import QtQuick3D

Item {
  id: board
  property int design: backend.pieceSet
  property bool motion: !backend.reducedMotion && Window.window && Window.window.active
  property real azimuth: backend.computer ? (backend.humanWhite ? 0 : 180) : (backend.autoRotate && !backend.whiteTurn ? 180 : 0)
  property real elevation: -49
  property real distance: 13.8
  property int cursorSquare: 12
  property bool keyboardMode: false
  property real checkGlow: .22
  property var lightSquares: ["#929a9f", "#dec9a2", "#d7d5b7", "#f1ded5"]
  property var darkSquares: ["#394652", "#654333", "#65775d", "#a78eb3"]
  property var frames: ["#28343f", "#432b22", "#5b4934", "#e8c6be"]
  property var accents: ["#dcad62", "#cba265", "#9baa6b", "#dca6ab"]
  focus: true
  function resetCamera() {
    azimuth = backend.computer ? (backend.humanWhite ? 0 : 180) : 0
    elevation = -49
    distance = 13.8
  }
  function keyboardSelect() { keyboardMode = true; backend.selectSquare(cursorSquare) }
  function navigate(dx, dy) {
    keyboardMode = true
    let x = Math.max(0, Math.min(7, cursorSquare % 8 + dx))
    let y = Math.max(0, Math.min(7, Math.floor(cursorSquare / 8) + dy))
    cursorSquare = y * 8 + x
  }
  Behavior on azimuth { enabled: board.motion && !orbit.dragging; NumberAnimation { duration: 650; easing.type: Easing.InOutCubic } }
  Connections {
    target: backend
    function onChanged() {
      if (backend.autoRotate && !backend.computer) board.azimuth = backend.whiteTurn ? 0 : 180
    }
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
      GradientStop { position: 0; color: theme.light ? Qt.lighter(theme.background,1.04) : "#1e2934" }
      GradientStop { position: 1; color: theme.background }
    }
  }
  View3D {
    id: scene
    anchors.fill: parent
    environment: SceneEnvironment {
      backgroundMode: SceneEnvironment.Transparent
      antialiasingMode: SceneEnvironment.MSAA
      antialiasingQuality: SceneEnvironment.High
      tonemapMode: SceneEnvironment.TonemapModeAces
      aoEnabled: true
      aoStrength: 35
      aoDistance: .8
      aoSoftness: 50
    }
    Node {
      eulerRotation: Qt.vector3d(board.elevation,board.azimuth,0)
      PerspectiveCamera { id: camera; z: board.distance; clipNear: .1; clipFar: 150; fieldOfView: 43 }
    }
    camera: camera
    DirectionalLight {
      eulerRotation: Qt.vector3d(-45,-35,0)
      color: "#ffebd2"
      brightness: 1.6
      castsShadow: true
      shadowMapQuality: Light.ShadowMapQualityHigh
      shadowFactor: 55
      shadowBias: .015
      shadowMapFar: 40
    }
    DirectionalLight { eulerRotation: Qt.vector3d(-30,140,0); color: "#c2ddf4"; brightness: .85 }
    DirectionalLight { eulerRotation: Qt.vector3d(-70,0,0); color: "#ffffff"; brightness: .45 }
    Model {
      source: "#Cube"
      y: -.27
      scale: Qt.vector3d(.091,.005,.091)
      materials: PrincipledMaterial { baseColor: board.frames[board.design]; roughness: .38; metalness: board.design === 0 ? .4 : .05 }
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
      materials: PrincipledMaterial { baseColor: board.frames[board.design]; roughness: .4 }
    }
    Repeater3D {
      model: 64
      Model {
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
          baseColor: parent.marked ? board.accents[board.design] : (Math.floor(index / 8) + index % 8) % 2 === 1 ? board.lightSquares[board.design] : board.darkSquares[board.design]
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
        board.azimuth -= (mouse.x - previousX) * .4
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
    Text { text: qsTr("Right-drag to orbit"); color: theme.foreground; opacity: .5; font.pixelSize: 11 }
    Text { text: qsTr("Scroll to zoom"); color: theme.foreground; opacity: .5; font.pixelSize: 11 }
    Text { text: qsTr("F · Flip board"); color: theme.foreground; opacity: .5; font.pixelSize: 11 }
  }
}
