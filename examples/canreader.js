var fs = require('fs');
var readline = require('readline');
const debug = console.log; // Set to false to remove debug

const can0pipePath = '/var/run/rexgen/can0/rx';
const can1pipePath = '/var/run/rexgen/can1/rx';
const accpipePath = '/var/run/rexgen/acc/rx';
const gyropipePath = '/var/run/rexgen/gyro/rx';

var outputstream = process.stdout;

function can(pipeName, channel)
{
	this.pipeName = pipeName;
	this.channel = channel;
	this.channelname = 'can' + channel;

	this.lineReader = readline.createInterface({ input: fs.createReadStream(this.pipeName) });

	this.lineReader.on('line', line => { this.onread(line); });
	this.lineReader.on('pause', () => { console.log('Readline paused.'); });
	this.lineReader.on('close', () => { console.log('Readline close.');});
}

can.prototype.pipe = function (output)
{
	this.pipeOut = output;
	this.lineReader.resume();
}

can.prototype.onread = function (line)
{
	const arr = line.split(/\s+/);
	if (arr.length < 4)
		return;

	this['timestamp'] = parseFloat(arr[0].split(/[()]/)[1]);
	if (arr[1] != this.channelname)
		return;

	this['channel'] = this.channel;
	this['ident'] = Number('0x'+arr[2]);
	this['IDE'] = (arr[2].length > 3);
	this['DLC'] = Number(arr[3].split(/[\[\]]/)[1]);
	this['Data'] = arr.slice(4, 4 + this['DLC']).join('');

	var str = 
		'Timestamp: ' + this['timestamp'] + ', ' +
		'Channel: ' + this['channel'] + ', ' +
		'Ident: ' + this['ident'] + ', ' +
		(this['IDE'] ? "X " : "") +
		'DLC: ' + this['DLC'] + ', ' +
		'Data: ' + this['Data'];

	//debug && debug('data: ' + JSON.stringify(arr));
	this.pipeOut.write(str + '\n');

}

async function startCanReader(canfifo, channel)
{
	var fifo = new can(canfifo, channel);
	while (true)
	{
		fifo.pipe(outputstream);
		debug && debug('fifo ' + channel + ' finish, waiting...');
		await new Promise((resolve) => { setTimeout(resolve, 1000); });
	}
}

function sensor(pipeName, sensorName)
{
	this.pipeName = pipeName;
	this.sensorname = sensorName;

	this.lineReader = readline.createInterface({ input: fs.createReadStream(this.pipeName) });

	this.lineReader.on('line', line => { this.onread(line); });
	this.lineReader.on('pause', () => { console.log('Readline paused.'); });
	this.lineReader.on('close', () => { console.log('Readline close.');});
}

sensor.prototype.pipe = function (output)
{
	this.pipeOut = output;
	this.lineReader.resume();
}

sensor.prototype.onread = function (line)
{
	const arr = line.split(/\s+/);
	if (arr.length < 3)
		return;
	if (arr[1] == 'NA')
		return;

	this['timestamp'] = parseFloat(arr[0].split(/[()]/)[1]);

	this['channelname'] = this.sensorname + arr[1];
	this['value'] = parseFloat(arr[2]);

	var str = 
		'Timestamp: ' + this['timestamp'] + ', ' +
		'Channel: ' + this['channelname'] + ', ' +
		'Value: ' + this['value'];

	//debug && debug('data: ' + JSON.stringify(arr));
	this.pipeOut.write(str + '\n');

}

async function startSensorReader(fifopath, sensorName)
{
	var fifo = new sensor(fifopath, sensorName);
	while (true)
	{
		fifo.pipe(outputstream);
		debug && debug('fifo ' + sensorName + ' finish, waiting...');
		await new Promise((resolve) => { setTimeout(resolve, 1000); });
	}

}

async function run()
{

	var arg = process.argv;
	if (arg.length > 3)
	{
		outputstream = fs.createWriteStream(arg[2]);
	}

	//new Promise((resolve) => { startCanReader(can0pipePath, 0); });
	//new Promise((resolve) => { startCanReader(can1pipePath, 1); });
	new Promise((resolve) => { startSensorReader(accpipePath, "Accelerometer"); });
	new Promise((resolve) => { startSensorReader(gyropipePath, "Gyroscope"); });
}

run();
