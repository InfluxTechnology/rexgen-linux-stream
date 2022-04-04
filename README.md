# rexgen-stream
Sample code for using ReXgen stream in Linux.

Usage:<br/>
node gnss2can.js can0<br/>
node gnss2can.js can1<br/>
node gnss2can.js myfile<br/>
node gnss2can.js > /path/to/output<br/>

canreader.js outputs data to default output - console (process.stdout)<br/>
It can be redirected using standard linux flow:<br/>
canreader.js > myfile<br/>


This line is sending gnss to rx fifo of rexgen, and simultaneously run canreader.js to output what is read.<br/>
node gnss2can.js > /var/run/rexgen/can0/rx & node canreader.js<br/>