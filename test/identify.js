var fprint = require("../index");

var ret = fprint.init()
if(ret) {
    fprint.setDebug(3);
    var devices = fprint.discoverDevices();
    if(devices.length > 0) {
        devices.forEach(function(entry) {
            console.log("Found: " + entry);
        });
        var prints = new Array();
        var deviceHandle = fprint.openDevice(devices[0]);

        function identify() {
            console.log("identify your finger! Please swipe your finger once again.")
            fprint.identifyStart(deviceHandle, prints, function(state, message, index) {
                console.log(message);
                if(state == 1 || state == 0) {
                    if(state == 1)
                        console.log("MATCHED.");
                    else
                        console.log("MATCH FAILED.");
                    fprint.identifyStop(deviceHandle, function () {
                        fprint.closeDevice(deviceHandle);
                        fprint.exit();
                    });
                }
                else {
                    console.log("Try again please. State: " + state);
                }
            });
        }

        function enroll(finger) {
            var stage = 1;
            var stages = fprint.getEnrollStages(deviceHandle);
            console.log("enroll your finger! You will need swipe your finger " + stages + " times.");
            console.log("stage " + stage++ + "/" + stages);

            fprint.enrollStart(deviceHandle, function(state, message, fingerprint) {
                console.log(message + "\n");
                if(state == 3) {
                    console.log("stage " + stage++ + "/" + stages);
                }
                else if(state == 1 || state == 2) {
                    if(state == 1) {
                        finger--;
                        prints[prints.length] = fingerprint;
                    }
                    fprint.enrollStop(deviceHandle, function() {
                        if(finger > 0)
                            enroll(finger);
                        else
                            identify();
                    });
                }
                else {
                    console.log("Try again please. State: " + state);
                }
            });
        }

        enroll(3);
    }
}
