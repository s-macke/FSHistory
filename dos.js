let game = {};
let screen;
let ram;
let graphics = {};
let doStopLoop = true;

function getScancode(key) {
    //alert(key);
    switch (key) {
        case "Escape":
            return 0x01; // esc
        case "F1":
            return 0x3b;
        case "F2":
            return 0x3c;
        case "F3":
            return 0x3d;
        case "F4":
            return 0x3e;
        case "F5":
            return 0x3f;
        case "F6":
            return 0x40;
        case "F7":
            return 0x41;
        case "F8":
            return 0x42;
        case "F9":
            return 0x43;
        case "F10":
            return 0x44;
        case "F11":
            return 0x57;
        case "F12":
            return 0x58;

        case "ScrollLock":
            return 0x46;
        case "Pause":
            return -1; // not supported

        case "Backquote":
            return 0x29; // 1
        case "Digit1":
            return 0x02; // 1
        case "Digit2":
            return 0x03; // 2
        case "Digit3":
            return 0x04; // 3
        case "Digit4":
            return 0x05; // 4
        case "Digit5":
            return 0x06; // 5
        case "Digit6":
            return 0x07; // 6
        case "Digit7":
            return 0x08; // 7
        case "Digit8":
            return 0x09; // 8
        case "Digit9":
            return 0x0a; // 9
        case "Digit0":
            return 0x0b; // 0
        case "KeyA":
            return 0x1e; // a
        case "KeyB":
            return 0x30; // b
        case "KeyC":
            return 0x2e; // c
        case "KeyD":
            return 0x20; // d
        case "KeyE":
            return 0x12; // e
        case "KeyF":
            return 0x21; // f
        case "KeyG":
            return 0x22; // g
        case "KeyH":
            return 0x23; // h
        case "KeyI":
            return 0x17; // i
        case "KeyJ":
            return 0x24; // j
        case "KeyK":
            return 0x25; // k
        case "KeyL":
            return 0x26; // l
        case "KeyM":
            return 0x32; // m
        case "KeyN":
            return 0x31; // n
        case "KeyO":
            return 0x18; // o
        case "KeyP":
            return 0x19; // p
        case "KeyQ":
            return 0x10; // q
        case "KeyR":
            return 0x13; // r
        case "KeyS":
            return 0x1f; // s
        case "KeyT":
            return 0x14; // t
        case "KeyU":
            return 0x16; // u
        case "KeyV":
            return 0x2f; // v
        case "KeyW":
            return 0x11; // w
        case "KeyX":
            return 0x2d; // x
        case "KeyY":
            return 0x2c; // y
        case "KeyZ":
            return 0x15; // z
        case "ShiftLeft":
            return 0x2a; // left shift
        case "ShiftRight":
            return 0x36; // right shift
        case "Space":
            return 0x39; // space
        case "Enter":
            return 0x1c; // enter
        case "Backspace":
            return 0xe; // backspace
        case "ControlLeft":
            return 0x1d; // left ctrl
        case "ControlRight":
            return 0x1d; // right ctrl
        case "ArrowUp":
            return 0x48; // cursor up
        case "ArrowDown":
            return 0x50; // cursor down
        case "ArrowLeft":
            return 0x4b; // cursor left
        case "ArrowRight":
            return 0x4d; // cursor right
        case "Insert":
            return 0x52; // ins
        case "Home":
            return 0x47; // home
        case "PageUp":
            return 0x49; // pg up
        case "Delete":
            return 0x53; // del
        case "End":
            return 0x4f; // end
        case "PageDown":
            return 0x51; // pg down
        case "NumLock":
            return 0x45; // num lock
        case "NumpadDivide":
            return 0x35; // num pad divide
        case "NumpadMultiply":
            return 0x37; // num pad multiply
        case "NumpadSubtract":
            return 0x4a; // num pad subtract
        case "NumpadAdd":
            return 0x4e; // num pad add
        case "NumpadEnter":
            return 0x1c; // enter
        case "Numpad0":
            return 0x52; // 0
        case "NumpadDecimal":
            return 0x53; //
        case "Numpad1":
            return 0x4f; // 1
        case "Numpad2":
            return 0x50; // 2
        case "Numpad3":
            return 0x51; // 3
        case "Numpad4":
            return 0x4b; // 4
        case "Numpad5":
            return 0x4c; // 5
        case "Numpad6":
            return 0x4d; // 6
        case "Numpad7":
            return 0x47; // 7
        case "Numpad8":
            return 0x48; // 8
        case "Numpad9":
            return 0x49; // 9
        case "Minus":
            return 0x0c; // -
        case "Equal":
            return 0x0d; // =
        case "BracketLeft":
            return 0x1a; // [
        case "BracketRight":
            return 0xab; // ]
        case "Semicolon":
            return 0x27; // ;
        case "Quote":
            return 0x28; // '
        case "Backslash":
            return 0x2b; // \
        case "Comma":
            return 0x33; // ,
        case "Period":
            return 0x34; // .
        case "Slash":
            return 0x35; // /
        case "IntlBackslash":
            return 0x56; // <
        case "ContextMenu":
            return -1;
        case "AltRight":
            return 38;
    }
    console.log("Unknown key " + key);
    return -1;
}

function DetectKeysDown(e) {
    e.preventDefault();
    //if (e.repeat) return;
    let scancode = getScancode(e.code);
    if (scancode === -1) return;
    game.KeyDown(scancode);
}

function DetectKeysUp(e) {
    //if (e.repeat) return;
    e.preventDefault();
    let scancode = getScancode(e.code);
    if (scancode === -1) return;
    game.KeyUp(scancode);
}

function GetMousePosition(e)
{
    // fix for Chrome
    if (e.type.startsWith('touch'))
    {
        return [e.targetTouches[0].pageX, e.targetTouches[0].pageY];
    } else
    {
        return [e.pageX, e.pageY];
    }
}
/*
function getMouseRelativeToCanvas(canvas, position) {
    let rect = canvas.getBoundingClientRect();
    return {
        x: evt.clientX - rect.left,
        y: evt.clientY - rect.top
    };
}
*/

function getMouseRelativeToCanvas(canvas, position) {
    let rect = canvas.getBoundingClientRect();
    return {
        x: (position[0] - rect.left) / rect.width,
        y: (position[1] - rect.top) / rect.height
    };
}

function DetectMouseMove(e) {
    e.preventDefault();
    let currentMousePosition = GetMousePosition(e);
    let normalizedPosition = getMouseRelativeToCanvas(graphics.scaledcanvas, currentMousePosition);
    game.MouseMotion(normalizedPosition.x*graphics.canvas.width, normalizedPosition.y*graphics.canvas.height);
}

function DetectMouseUp(e) {
    game.MouseButtonUp(1);
}

function DetectMouseDown(e) {
    game.MouseButtonDown(1);
}

function ReinitGraphics() {
    graphics.canvas = document.createElement('canvas');
    graphics.mode = game.VGA_GetVideoMode();
    switch(graphics.mode) {
        case 0x0:
        case 0x2:
        case 0x3:
        case 0x6:
            graphics.canvas.width = 640;
            graphics.canvas.height = 200;
            break;

        case 0x1:
            graphics.canvas.width = 320;
            graphics.canvas.height = 400;
            break;
        case 0x7:
        case 0x14:
            graphics.canvas.width = 720;
            graphics.canvas.height = 400;
            break;

        case 0x4:
        case 0xd:
        case 0x13:
            graphics.canvas.width = 320;
            graphics.canvas.height = 200;
            break;
        case 0x10:
            graphics.canvas.width = 640;
            graphics.canvas.height = 350;
            break;

        default:
            graphics.canvas.width = 640;
            graphics.canvas.height = 480;
            break;
    }

    graphics.ctx = graphics.canvas.getContext("2d");
    graphics.data = graphics.ctx.createImageData(graphics.canvas.width, graphics.canvas.height);
}

function resizeCanvas() {
    //graphics.scaledcanvas.style.height = window.innerHeight + "px";
    //graphics.scaledcanvas.style.width = window.innerWidth + "px";

    let viewwidth  = window.innerWidth || document.documentElement.clientWidth || document.body.clientWidth;
    let viewheight = window.innerHeight|| document.documentElement.clientHeight || document.body.clientHeight;

    viewwidth *= 3./4.; // grid fraction
    let ratio = viewwidth/viewheight;
    if (ratio > 4./3.) {
        graphics.scaledcanvas.style.width = "auto";
        graphics.scaledcanvas.style.height = "100%";
    } else {
        graphics.scaledcanvas.style.width = "100%";
        graphics.scaledcanvas.style.height = "auto";
    }

    /*

    let height = graphics.scaledcanvas.innerWidth * 0.75;
    if (viewheight < height) {
        alert("be careful");
        graphics.scaledcanvas.style.height = window.innerHeight + "px";
        graphics.scaledcanvas.style.width = ((window.innerHeight*4./3.)|0) + "px";
    } else {
        graphics.scaledcanvas.style.width = "100%";
        graphics.scaledcanvas.style.height = "auto";
    }
    */
}

function Init() {
    graphics.scaledcanvas = document.getElementById("screen");
    graphics.scaledcanvas.width = 640;
    graphics.scaledcanvas.height = 480;
    graphics.scaledctx = graphics.scaledcanvas.getContext("2d");
    window.addEventListener('resize', resizeCanvas, false);
    resizeCanvas();

    ReinitGraphics();
    window.onkeydown    = DetectKeysDown;
    window.onkeyup      = DetectKeysUp;
    graphics.scaledcanvas.onmousemove  = DetectMouseMove;
    graphics.scaledcanvas.onmousedown  = DetectMouseDown;
    graphics.scaledcanvas.onmouseup    = DetectMouseUp;
    window.ontouchstart = DetectMouseDown;
    window.ontouchend   = DetectMouseUp;
    window.ontouchmove  = DetectMouseMove;
}

let lastDate = null;
// returns the time since the last call
function getDeltaMs() {
    if (lastDate === null) {
        lastDate = new Date();
        return 0;
    }
    let lastTime = lastDate.getTime();
    lastDate = new Date();
    return lastDate.getTime() - lastTime;
}

function Loop() {
    if (doStopLoop) return;

    let delta = getDeltaMs();
    if (delta > 60) delta = 60; // just make sure the browser is not locked forever
    game.Run(delta*5000);
    //game.Run(500000);
    game.UpdateScreen();

    if (graphics.mode != game.VGA_GetVideoMode()) ReinitGraphics();
    let width = graphics.canvas.width;
    let height = graphics.canvas.height;
    for (let j = 0; j < height; j++) {
        let offsets = j * 720 * 4;
        let offsetg = j * width * 4;
        for (let i = 0; i < width * 4; i++) {
            graphics.data.data[offsetg++] = screen[offsets++];
        }
    }
    graphics.ctx.putImageData(graphics.data, 0, 0);
    graphics.scaledctx.drawImage(
        graphics.canvas,
        0, 0, graphics.canvas.width, graphics.canvas.height,
        0, 0, graphics.scaledcanvas.width, graphics.scaledcanvas.height);

    window.requestAnimationFrame(Loop);
    //window.setTimeout(Loop, 0);
}

let str = "";

function Stdout(strp) {
    for (let i = 0; i < 256; i++) {
        if (ram[strp + i] === 0) break;
        if (ram[strp + i] < 0x20) {
            if (ram[strp + i] === 0x0A) {
                console.log(str);
                str = "";
                continue;
            }
        }
        str += String.fromCharCode(ram[strp + i]);
    }
}

function MountFs(fsAsByteArray) {

    let filesystem = new Uint8Array(fsAsByteArray);
    let uncompressed = [];
    console.log("decompress filesystem of size " + fsAsByteArray.byteLength);
    bzip2.simple(filesystem, function(byte){uncompressed.push(byte)});
    console.log("decompressed filesystem of size " + uncompressed.length);
    let fsaddress = game.GetMountStorage(uncompressed.length);
    for(let i=0;i<uncompressed.length;i++) ram[fsaddress+i] = uncompressed[i];
    game.FinishMountStorage();
}

function Main(version) {
    doStopLoop = true;
    let importOb = {
        env: {
            memory: new WebAssembly.Memory({initial: 512, maximum: 512}), // 8 MB
            exit: function (code) {
                console.log("Exit code " + code);
                console.log("Restart");
                Main(version);
            },
            outputstr: Stdout
        }
    };

    Promise
        .all([fetch('fshistory.wasm'), fetch('data/fs'+version+'.fs.bz2')])
        .then(responses => Promise.all([responses[0].arrayBuffer(), responses[1].arrayBuffer()]))
        .then(data => Promise.all([WebAssembly.instantiate(data[0], importOb), Promise.resolve(data[1])])
    ).then(
        objs => {
            console.log("wasm file loaded");
            game = objs[0].instance.exports;
            ram = new Uint8Array(importOb.env.memory.buffer, 0);
            game.Init();
            MountFs(objs[1]);

            game.SetFSVersion(version);
            screen = new Uint8Array(importOb.env.memory.buffer, game.ScreenGet());
            Init();
            console.log("Initialization completed. Starting Loop");
            doStopLoop = false;
            Loop();
            //alert(game.__heap_base.value);
            //alert(game.__global_base.value);
        },
        err => alert(err)
    );
}
