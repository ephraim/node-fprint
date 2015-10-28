var fprint = require("../index");


var ret = fprint.init()
if(ret) {
    fprint.setDebug(3);
    var devices = fprint.discoverDevices();
    devices.forEach(function(entry) {
        console.log("Found: " + entry);
    });
    var deviceHandle = fprint.openDevice(devices[0]);
    console.log("enroll your finger!");
    ret = fprint.enroll(deviceHandle);
    if(ret.result) {
        console.log("\n\nsucceeded! Now verifing it!");
        ret = fprint.verify(deviceHandle, ret.fingerprint);
    }
    fprint.closeDevice(deviceHandle);
    fprint.exit();
}
