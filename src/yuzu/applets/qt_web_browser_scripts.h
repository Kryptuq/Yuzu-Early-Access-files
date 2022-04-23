// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

constexpr char NX_FONT_CSS[] = R"(
(function() {
    css = document.createElement('style');
    css.type = 'text/css';
    css.id = 'nx_font';
    css.innerText = `
/* FontStandard */
@font-face {
    font-family: 'FontStandard';
    src: url('%1') format('truetype');
}

/* FontChineseSimplified */
@font-face {
    font-family: 'FontChineseSimplified';
    src: url('%2') format('truetype');
}

/* FontExtendedChineseSimplified */
@font-face {
    font-family: 'FontExtendedChineseSimplified';
    src: url('%3') format('truetype');
}

/* FontChineseTraditional */
@font-face {
    font-family: 'FontChineseTraditional';
    src: url('%4') format('truetype');
}

/* FontKorean */
@font-face {
    font-family: 'FontKorean';
    src: url('%5') format('truetype');
}

/* FontNintendoExtended */
@font-face {
    font-family: 'NintendoExt003';
    src: url('%6') format('truetype');
}

/* FontNintendoExtended2 */
@font-face {
    font-family: 'NintendoExt003';
    src: url('%7') format('truetype');
}
`;

    document.head.appendChild(css);
})();
)";

constexpr char LOAD_NX_FONT[] = R"(
(function() {
    var elements = document.querySelectorAll("*");

    for (var i = 0; i < elements.length; i++) {
        var style = window.getComputedStyle(elements[i], null);
        if (style.fontFamily.includes("Arial") || style.fontFamily.includes("Calibri") ||
            style.fontFamily.includes("Century") || style.fontFamily.includes("Times New Roman")) {
            elements[i].style.fontFamily = "FontStandard, FontChineseSimplified, FontExtendedChineseSimplified, FontChineseTraditional, FontKorean, NintendoExt003";
        } else {
            elements[i].style.fontFamily = style.fontFamily + ", FontStandard, FontChineseSimplified, FontExtendedChineseSimplified, FontChineseTraditional, FontKorean, NintendoExt003";
        }
    }
})();
)";

constexpr char FOCUS_LINK_ELEMENT_SCRIPT[] = R"(
if (document.getElementsByTagName("a").length > 0) {
    document.getElementsByTagName("a")[0].focus();
}
)";

constexpr char GAMEPAD_SCRIPT[] = R"(
window.addEventListener("gamepadconnected", function(e) {
    console.log("Gamepad connected at index %d: %s. %d buttons, %d axes.",
        e.gamepad.index, e.gamepad.id, e.gamepad.buttons.length, e.gamepad.axes.length);
});

window.addEventListener("gamepaddisconnected", function(e) {
    console.log("Gamepad disconnected from index %d: %s", e.gamepad.index, e.gamepad.id);
});
)";

constexpr char WINDOW_NX_SCRIPT[] = R"(
var end_applet = false;
var yuzu_key_callbacks = [];

(function() {
    class WindowNX {
        constructor() {
            yuzu_key_callbacks[1] = function() { window.history.back(); };
            yuzu_key_callbacks[2] = function() { window.nx.endApplet(); };
        }

        addEventListener(type, listener, options) {
            console.log("nx.addEventListener called, type=%s", type);

            window.addEventListener(type, listener, options);
        }

        endApplet() {
            console.log("nx.endApplet called");

            end_applet = true;
        }

        playSystemSe(system_se) {
            console.log("nx.playSystemSe is not implemented, system_se=%s", system_se);
        }

        sendMessage(message) {
            console.log("nx.sendMessage is not implemented, message=%s", message);
        }

        setCursorScrollSpeed(scroll_speed) {
            console.log("nx.setCursorScrollSpeed is not implemented, scroll_speed=%d", scroll_speed);
        }
    }

    class WindowNXFooter {
        setAssign(key, label, func, option) {
            console.log("nx.footer.setAssign called, key=%s", key);

            switch (key) {
                case "A":
                    yuzu_key_callbacks[0] = func;
                    break;
                case "B":
                    yuzu_key_callbacks[1] = func;
                    break;
                case "X":
                    yuzu_key_callbacks[2] = func;
                    break;
                case "Y":
                    yuzu_key_callbacks[3] = func;
                    break;
                case "L":
                    yuzu_key_callbacks[6] = func;
                    break;
                case "R":
                    yuzu_key_callbacks[7] = func;
                    break;
            }
        }

        setFixed(kind) {
            console.log("nx.footer.setFixed is not implemented, kind=%s", kind);
        }

        unsetAssign(key) {
            console.log("nx.footer.unsetAssign called, key=%s", key);

            switch (key) {
                case "A":
                    yuzu_key_callbacks[0] = function() {};
                    break;
                case "B":
                    yuzu_key_callbacks[1] = function() {};
                    break;
                case "X":
                    yuzu_key_callbacks[2] = function() {};
                    break;
                case "Y":
                    yuzu_key_callbacks[3] = function() {};
                    break;
                case "L":
                    yuzu_key_callbacks[6] = function() {};
                    break;
                case "R":
                    yuzu_key_callbacks[7] = function() {};
                    break;
            }
        }
    }

    class WindowNXPlayReport {
        incrementCounter(counter_id) {
            console.log("nx.playReport.incrementCounter is not implemented, counter_id=%d", counter_id);
        }

        setCounterSetIdentifier(counter_id) {
            console.log("nx.playReport.setCounterSetIdentifier is not implemented, counter_id=%d", counter_id);
        }
    }

    window.nx = new WindowNX();
    window.nx.footer = new WindowNXFooter();
    window.nx.playReport = new WindowNXPlayReport();
})();
)";
