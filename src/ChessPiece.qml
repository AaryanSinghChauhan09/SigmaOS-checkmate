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
  // Moss & Myth is alive: its figures breathe, perk up when chosen, watch a neighbour being picked, and leap rather than slide.
  required property real clock
  required property int attention
  property int square: record.square
  // Where the figure stands. It follows `square` by hand so a leap is measured before the slide starts.
  property int place: 0
  property int travel: 360
  property real raise: selected ? (woodland ? .17 : .1) : 0
  property real stretch: 1
  property real lean: 0
  property real leanX: 0
  property real leanZ: 0
  property real wiggle: 0
  property real vigor: woodland && motion ? 1 : 0
  // A tree stump stirs far less than the creatures around it.
  property real liveliness: record.kind === 4 ? .3 : 1
  property real phase: pieceId * 2.399
  property real breath: vigor * liveliness * Math.sin(clock * (1.5 + pieceId % 5 * .11) + phase)
  property real tall: stretch * (1 + .011 * breath * (1 + raise * 6))
  property real wide: 1 + (1 - tall) * .6
  // Every figure faces whoever is seated at the board. Knights turn in toward its middle: the foxes
  // a little, the horses far enough to show their profile.
  property real knightTurn: woodland ? 32 : 58
  property real facing: 180 + seat + (record.kind === 2 ? (pieceId % 2 ? knightTurn : -knightTurn) * Math.cos(seat * Math.PI / 180) : 0)
  // Neighbours turn part of the way toward the figure in hand; one standing behind them is left alone.
  property real look: regard()
  function regard() {
    if (!woodland || !motion || attention < 0 || selected || !record.alive) return 0
    let dx = attention % 8 - place % 8, dz = Math.floor(place / 8) - Math.floor(attention / 8)
    let reach = Math.hypot(dx, dz)
    if (reach < .5 || reach > 3.3) return 0
    return 20 * liveliness * Math.sin(Math.atan2(-dx, -dz) - facing * Math.PI / 180) * Math.min(1, (3.3 - reach) / 2)
  }
  x: place % 8 - 3.5
  z: 3.5 - Math.floor(place / 8)
  y: .12 + lift + raise * (1 + vigor * .1 * Math.sin(clock * 3.1 + phase))
  scale: Qt.vector3d(presence * wide, presence * tall, presence * wide)
  // A captured woodland figure twirls as it goes.
  eulerRotation.y: facing + wiggle + look + vigor * liveliness * 1.2 * Math.sin(clock * .7 + phase * 1.7) + (woodland ? (1 - presence) * 160 : 0)
  eulerRotation.x: lean * leanZ
  eulerRotation.z: -lean * leanX
  Behavior on x {
    id: xBehavior; enabled: piece.ready && piece.motion
    SequentialAnimation {
      PauseAnimation { duration: piece.woodland ? 70 : 0 }
      NumberAnimation { duration: piece.travel; easing.type: piece.woodland ? Easing.InOutSine : Easing.InOutCubic }
    }
  }
  Behavior on z {
    id: zBehavior; enabled: piece.ready && piece.motion
    SequentialAnimation {
      PauseAnimation { duration: piece.woodland ? 70 : 0 }
      NumberAnimation { duration: piece.travel; easing.type: piece.woodland ? Easing.InOutSine : Easing.InOutCubic }
    }
  }
  Behavior on raise {
    id: raiseBehavior; enabled: piece.ready && piece.motion
    NumberAnimation { duration: piece.woodland ? 300 : 180; easing.type: piece.woodland && piece.selected ? Easing.OutBack : Easing.OutCubic }
  }
  Behavior on look {
    id: lookBehavior; enabled: piece.ready && piece.motion
    SequentialAnimation {
      PauseAnimation { duration: piece.pieceId * 53 % 140 }
      NumberAnimation { duration: 380; easing.type: Easing.OutBack }
    }
  }
  // The captured figure waits for its captor to arrive.
  Behavior on presence {
    id: presenceBehavior; enabled: piece.ready && piece.motion
    SequentialAnimation {
      PauseAnimation { duration: piece.woodland && !piece.record.alive ? 240 : 0 }
      NumberAnimation { duration: piece.woodland ? 300 : 220; easing.type: Easing.InCubic }
    }
  }
  onMotionChanged: {
    if (!motion) {
      xBehavior.enabled = false; zBehavior.enabled = false
      raiseBehavior.enabled = false; lookBehavior.enabled = false; presenceBehavior.enabled = false
      hop.stop(); leap.stop(); perk.stop(); glance.stop(); skip.stop()
      lift = 0; stretch = 1; lean = 0; wiggle = 0
      // Resolve moves queued before the first animation frame as well.
      x = Qt.binding(function() { return piece.place % 8 - 3.5 })
      z = Qt.binding(function() { return 3.5 - Math.floor(piece.place / 8) })
      raise = Qt.binding(function() { return piece.selected ? (piece.woodland ? .17 : .1) : 0 })
      look = Qt.binding(function() { return piece.regard() })
      presence = Qt.binding(function() { return piece.record.alive ? 1 : 0 })
      xBehavior.enabled = Qt.binding(function() { return piece.ready && piece.motion })
      zBehavior.enabled = Qt.binding(function() { return piece.ready && piece.motion })
      raiseBehavior.enabled = Qt.binding(function() { return piece.ready && piece.motion })
      lookBehavior.enabled = Qt.binding(function() { return piece.ready && piece.motion })
      presenceBehavior.enabled = Qt.binding(function() { return piece.ready && piece.motion })
    }
  }
  onSquareChanged: {
    if (ready && motion && woodland && record.alive) {
      let dx = square % 8 - 3.5 - x, dz = 3.5 - Math.floor(square / 8) - z, reach = Math.hypot(dx, dz)
      if (reach > .01) {
        // Further is slower and higher; foxes clear whatever stands between.
        travel = Math.min(620, 300 + 50 * reach)
        leap.height = record.kind === 2 ? .55 : Math.min(.36, .12 + .04 * reach)
        // The figure leans along its path, which is turned into its own frame.
        let turn = eulerRotation.y * Math.PI / 180
        leanX = (dx * Math.cos(turn) - dz * Math.sin(turn)) / reach
        leanZ = (dx * Math.sin(turn) + dz * Math.cos(turn)) / reach
        perk.stop(); skip.stop()
        leap.restart()
      }
    } else travel = 360
    place = square
  }
  onSelectedChanged: if (selected && woodland && motion && ready) { skip.stop(); glance.stop(); perk.restart() }
  Component.onCompleted: { place = square; ready = true }
  // Now and then the board asks an idle figure to stir: a look to one side, or a small skip from the young and the foxes.
  function fidget() {
    if (!woodland || !motion || !record.alive || record.kind === 4 || selected || look !== 0) return
    if (leap.running || perk.running || glance.running || skip.running) return
    if (record.kind <= 2 && Math.random() < .4) skip.restart()
    else { glance.side = Math.random() < .5 ? -1 : 1; glance.restart() }
  }
  Connections {
    target: backend
    function onMoved(id, knight, capture) {
      if (id === piece.pieceId && piece.motion && !piece.woodland) {
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
  // A crouch, an arc leaning into the journey, and a soft landing that springs back.
  SequentialAnimation {
    id: leap
    objectName: "leap"
    property real height: .2
    NumberAnimation { target: piece; property: "stretch"; to: .9; duration: 70; easing.type: Easing.OutQuad }
    ParallelAnimation {
      SequentialAnimation {
        NumberAnimation { target: piece; property: "lift"; to: leap.height; duration: piece.travel * .5; easing.type: Easing.OutQuad }
        NumberAnimation { target: piece; property: "lift"; to: 0; duration: piece.travel * .5; easing.type: Easing.InQuad }
      }
      SequentialAnimation {
        NumberAnimation { target: piece; property: "stretch"; to: 1.07; duration: piece.travel * .3; easing.type: Easing.OutQuad }
        NumberAnimation { target: piece; property: "stretch"; to: 1; duration: piece.travel * .7 }
      }
      SequentialAnimation {
        NumberAnimation { target: piece; property: "lean"; to: 9; duration: piece.travel * .35; easing.type: Easing.OutQuad }
        NumberAnimation { target: piece; property: "lean"; to: -5; duration: piece.travel * .65; easing.type: Easing.InOutQuad }
      }
    }
    ParallelAnimation {
      SequentialAnimation {
        NumberAnimation { target: piece; property: "stretch"; to: .88; duration: 70; easing.type: Easing.OutQuad }
        NumberAnimation { target: piece; property: "stretch"; to: 1; duration: 320; easing.type: Easing.OutBack }
      }
      NumberAnimation { target: piece; property: "lean"; to: 0; duration: 280; easing.type: Easing.OutBack }
    }
  }
  // Picked up: a start, a stretch, and a shake of the head.
  ParallelAnimation {
    id: perk
    objectName: "perk"
    SequentialAnimation {
      NumberAnimation { target: piece; property: "stretch"; to: .93; duration: 70; easing.type: Easing.OutQuad }
      NumberAnimation { target: piece; property: "stretch"; to: 1.08; duration: 150; easing.type: Easing.OutQuad }
      NumberAnimation { target: piece; property: "stretch"; to: 1; duration: 260; easing.type: Easing.OutBack }
    }
    SequentialAnimation {
      PauseAnimation { duration: 120 }
      NumberAnimation { target: piece; property: "wiggle"; to: 9; duration: 110; easing.type: Easing.InOutSine }
      NumberAnimation { target: piece; property: "wiggle"; to: -7; duration: 150; easing.type: Easing.InOutSine }
      NumberAnimation { target: piece; property: "wiggle"; to: 0; duration: 220; easing.type: Easing.OutBack }
    }
  }
  SequentialAnimation {
    id: glance
    property int side: 1
    NumberAnimation { target: piece; property: "wiggle"; to: glance.side * 15; duration: 380; easing.type: Easing.InOutSine }
    PauseAnimation { duration: 500 }
    NumberAnimation { target: piece; property: "wiggle"; to: 0; duration: 520; easing.type: Easing.InOutSine }
  }
  ParallelAnimation {
    id: skip
    SequentialAnimation {
      NumberAnimation { target: piece; property: "lift"; to: .07; duration: 130; easing.type: Easing.OutQuad }
      NumberAnimation { target: piece; property: "lift"; to: 0; duration: 170; easing.type: Easing.InQuad }
    }
    SequentialAnimation {
      NumberAnimation { target: piece; property: "stretch"; to: 1.05; duration: 130 }
      NumberAnimation { target: piece; property: "stretch"; to: .94; duration: 170 }
      NumberAnimation { target: piece; property: "stretch"; to: 1; duration: 240; easing.type: Easing.OutBack }
    }
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
