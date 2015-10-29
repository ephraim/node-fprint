var fprint = require("../index");


var ret = fprint.init()
if(ret) {
    fprint.setDebug(3);
    var devices = fprint.discoverDevices();
    devices.forEach(function(entry) {
        console.log("Found: " + entry);
    });

    do {
        var deviceHandle = fprint.openDevice(devices[0]);
        var stage = 1;
        var stages = fprint.getEnrollStages(deviceHandle);
        console.log("enroll your finger! You will need swipe your finger " + stages + " times.");

        console.log("stage " + stage++ + "/" + stages);
        ret = fprint.enroll(deviceHandle, function(state, message) {
            console.log("state: " + state + "; message: " + message)
            if(state == 3) {
                console.log("stage " + stage++ + "/" + stages);
            }
        });
        if(ret.result) {
            console.log("\n\nsucceeded! Now verifing it!");
            ret = fprint.verify(deviceHandle, ret.fingerprint, function(state, message){
                console.log("state: " + state + "; message: " + message)
            });
            console.log(ret);
            break;
        }
        fprint.closeDevice(deviceHandle);
    }
    while(1);
    fprint.exit();
}
