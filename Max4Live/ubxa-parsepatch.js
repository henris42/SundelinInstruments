
function hsprocessPacket(packet) {
    if (packet[0] === 0x00 && packet[9] === 0x1) {
        return [];
    }
    if (packet[0] === 0x00 && packet[9] === 0x2) {
        var length = packet[11];
        return packet.slice(12, 12 + length);
    }
    return [];
}

function hsprocessBuf(data) {
    var abits = data[96];
    var patch = [];
    var bits = abits ^ 255;

    var b0 = (data[88] >> 1) & 1;
    var b1 = (data[88] >> 3) & 1;
    var b2 = (data[88] >> 5) & 1;
    var b3 = 0;
    var b4 = 2;
    var b5 = 4;
    var b6 = 6;
    var b7 = (data[104] >> 1) & 1;

    patch.push(data[91] * 2 + b0);
    patch.push(data[90]);

    patch.push(data[93] * 2 + b1);
    patch.push(data[92]);

    patch.push(data[95] * 2 + b2);
    patch.push(data[94]);

    patch.push(data[98] * 2 + ((abits >> b3) & 1));
    patch.push(data[97]);

    patch.push(data[100] * 2 + ((abits >> b4) & 1));
    patch.push(data[99]);

    patch.push(data[102] * 2 + ((abits >> b5) & 1));
    patch.push(data[101]);

    patch.push(data[105] * 2 + ((abits >> b6) & 1));
    patch.push(data[103]);

    patch.push(data[107] * 2 + b7);
    patch.push(data[106]);

    return patch.map(function (x) {
        return String.fromCharCode(x);
    }).join('');
}

function hsprocess(buf) {
    var curr = [];
    var data = [];

    for (var i = 0; i < buf.length; i++) {
        var c = buf[i];
        if (c === 0xf0) {
            curr = [];
            continue;
        }
        if (c === 0xf7) {
            var p = hsprocessPacket(curr);
            data = data.concat(p);
        }
        curr.push(c);
    }

    return hsprocessBuf(data);
}


function list() {
	s='';
	a=[];
		
	for(i in arguments) {
		//s=s+i[2].toString(16);
		//post(JSON.stringify(i)+" "+arguments[i]+"\n");
		a.push(arguments[i])
	}
	
	//post(a)
	
	pname=hsprocess(a)
	
   	post("Ub-Xa Current Patch name:"+pname+"\n");
	outlet(0,pname);
}

