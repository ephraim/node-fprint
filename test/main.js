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
        var stage = 1;
        var stages = fprint.getEnrollStages(deviceHandle);
        console.log("enroll your finger! You will need swipe your finger " + stages + " times.");
        console.log("stage " + stage++ + "/" + stages);
        ret = fprint.enrollStart(deviceHandle, function(state, message, fingerprint) {
            console.log("state: " + state + "; message: " + message)
            if(state == 3) {
                console.log("stage " + stage++ + "/" + stages);
            }
            else if(state == 1 || state == 2) {
                console.log(fingerprint);

                fprint.enrollStop(deviceHandle, function() {
                    console.log("closing down");
                    fprint.closeDevice(deviceHandle);
                    fprint.exit();
                });
            }
        });
    }
}
