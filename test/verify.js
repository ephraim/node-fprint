var fprint = require("../index");

var ret = fprint.init()
if(ret) {
    fprint.setDebug(3);
    var devices = fprint.discoverDevices();
    if(devices.length > 0) {
        devices.forEach(function(entry) {
            console.log("Found: " + entry);
        });
        var deviceHandle = fprint.openDevice(devices[0]);
        var stage = 1;
        var stages = fprint.getEnrollStages(deviceHandle);
        console.log("enroll your finger! You will need swipe your finger " + stages + " times.");
        console.log("stage " + stage++ + "/" + stages);
        ret = fprint.enrollStart(deviceHandle, function(state, message, fingerprint) {
            console.log(message + "\n");
            if(state == 3) {
                console.log("stage " + stage++ + "/" + stages);
            }
            else if(state == 1 || state == 2) {
                fprint.enrollStop(deviceHandle, function() {
                    if(state == 1) {
                        console.log("fingerprint size: " + fingerprint.length);
                        console.log("now your finger will be verified. Please swipe your finger once again.");
                    	fprint.verifyStart(deviceHandle, fingerprint, function(state, message) {
                            console.log(message);
                            if(state == 1 || state == 0) {
                                if(state == 1)
                                    console.log("MATCHED.");
                                else
                                    console.log("MATCH FAILED.");
                                fprint.verifyStop(deviceHandle, function () {
                                    fprint.closeDevice(deviceHandle);
                                    fprint.exit();
                                });
                            }
                            else {
                                console.log("Try again please. State: " + state);
                            }
                    	});
                    }
                    else {
                        fprint.closeDevice(deviceHandle);
                        fprint.exit();
                    }
                });
            }
            else {
                console.log("Try again please. State: " + state);
            }
        });
    }
}
