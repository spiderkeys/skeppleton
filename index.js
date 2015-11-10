
var addon = require('./build/Release/skeppleton');

console.log(addon.StartThread());

var command = 0;

setInterval(function() 
{
    addon.SendCommandToThread( command );
    command++;
},1000);