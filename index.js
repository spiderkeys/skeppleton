// var addon = require('./build/Release/skeppleton');

// var i = 0;

// setInterval(function() {
//   console.log(i++);
// },500);

// // test the delay function
// addon.delay(3,'hello world',function(a,b) {
//   console.log('delay : ' + a + ',' + b);
// });

var addon = require('./build/Release/skeppleton');

console.log(addon.StartThread());

// setInterval(function() 
// {
//   console.log( "Hi" );
// },1000);