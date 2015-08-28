var fs = require('fs');
var raspicam2 = require('bindings')('raspicam2.node')

var width = 640;
var height = 480;

raspicam2.setSize(width, height);
raspicam2.open();

var readFunc = function(data) {
    console.log("got data: " + data.length);
}

var startTime = new Date().getTime();
var count = 0;
setInterval(function() {
    raspicam2.readAsync(readFunc);

    var currTime = new Date().getTime();
    if (currTime - startTime > 5000) {
	console.log("fps: " + (count * 1000 / (currTime - startTime)));
	count = 0;
	startTime = currTime;
    }
    count++;
}, 30);