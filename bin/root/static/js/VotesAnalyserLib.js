var DEBUG = false;
var SEATMAP_WIDTH = 900;
var SEATMAP_HEIGHT = 850;
var DEFAULT_MAP_COLOR = 'gray';
var BUBBLE_RADIUS_BIG = 15
var BUBBLE_RADIUS = 12;

var idToSeatMapId = [];
var seatsByParty = [];
var wListener = 0;
var seatMap = null;
var seatList = [];
var textList = [];
var matchDataList = [];
var partyButtonsList = [];
var seatMapPartiesShown = null;
var matchLine = null;
var locale = null;
var voteStatistics = null;

function handleBubleCliked(element) {
	var id = parseInt(this.getAttribute('id'));
    Wt.emit(wListener, 'representativeCliked', id);
    var seat = id-1;
    positionRepresentativeBox(seat);
    setMatchLine(seat);
}

function handlePartyBubleCliked(element) {
	var partyId = parseInt(this.getAttribute('id').substr(6));
	if (DEBUG) {
		console.log('handlePartyBubleCliked: id: ' +  partyId);
	}
	for (var i = 0; i < seatMapPartiesShown.length; i++) {
		if (seatMapPartiesShown[i].id == partyId) {
			seatMapPartiesShown[i].shown = !seatMapPartiesShown[i].shown;
			var partyButtonIndex = partyId - 1;
			if (seatMapPartiesShown[i].shown) {
				partyButtonsList[partyButtonIndex].attr({fill : 'green'});
			} else {
				partyButtonsList[partyButtonIndex].attr({fill : 'red'});
			}
			
			if (DEBUG) {
				console.log(seatMapPartiesShown[i].name + " set to " + seatMapPartiesShown[i].shown); 
			}
			break;
		}
	}
	fadeNonSelectedPartymembers(seatList, seatsByParty, seatMapPartiesShown);
}

function drawVoteStatistics(data) {
	if (seatMap == null) {
		return ;
	}
		
	var statisticsHeigth = 150;
	var partyNamesStep = seatMap.width / (partyButtonsList.length + 1);
	if (voteStatistics != null) {
		voteStatistics.remove();
	}
	
	var percentages = [];
	for (var choise = 0; choise < 4; choise++) {
		var subList = []
		for (var i = 0; i < partyButtonsList.length; i++) {
			subList.push(data[choise + (i*4)] + 1);
		}
		percentages.push(subList)
	}
	
	if (DEBUG) {
		console.log('drawVoteStatistics');
		console.log(data);
		console.log(percentages);
	}
	
	var fin = function () {
		if (this.infoPopup == null) {
			this.infoPopup = seatMap.popup(this.bar.x, this.bar.y, this.bar.description + (this.bar.value - 1).toString()).insertBefore(this);
		} else {
			this.infoPopup.show();
			this.infoPopup.animate({opacity: 100}, 300);
		}
	};
	var fout = function () {
		this.infoPopup.animate({opacity: 0}, 300, function () {this.hide();});
	};
	
	voteStatistics = seatMap.barchart(partyNamesStep/2, SEATMAP_HEIGHT-statisticsHeigth, SEATMAP_WIDTH - partyNamesStep+20, statisticsHeigth, percentages, {colors:["green", "red", "black", "gray"]});
	voteStatistics.hover(fin, fout)
	for (var choise = 0; choise < voteStatistics.bars.length; choise++) {
		for (var party = 0; party < voteStatistics.bars[choise].length; party++) {	
			if (choise == 0) {
				voteStatistics.bars[choise][party].description = seatMapPartiesShown[party].name + ': ' + locale.yes + ' ' + locale.vote + ': ';
			} else if (choise == 1) {
				voteStatistics.bars[choise][party].description = seatMapPartiesShown[party].name + ': ' + locale.no + ' ' + locale.vote + ': ';
			} else if (choise == 2) {
				voteStatistics.bars[choise][party].description = seatMapPartiesShown[party].name + ': ' + locale.empty + ' ' + locale.vote + ': ';
			} else if (choise == 3) {
				voteStatistics.bars[choise][party].description = seatMapPartiesShown[party].name + ': ' + locale.away + ': ';
			} else {
				console.log('Lets pretend everything is ok');
			}
		}	
	}
}

function visualizeMatchWithMap(data) {
	if (seatMap == null) {
		return ;
	}	
	
	if (voteStatistics != null) {
		voteStatistics.remove();
	}
	
	dataArray = data.split(";");
	bestMatch = dataArray[0];
	
	if (DEBUG) {
		console.log("visualizeMatchWithMap with " + dataArray.length + " match values");
	}
	
	for (var i = 0; i < dataArray.length - 1; i++) {							//Discard last empty element
		values = dataArray[i].split('-')
		percentage = parseInt(values[0]);
		numberOfVotes = parseInt(values[1]);

		var g = 0;
		var r = 0;
		var b = 0;
	
		if (percentage < 10) {
			g = 0;
			r = 255;
		} else if (percentage < 30) {
			r = 253;
			g = 149;
			b = 19;
		} else if (percentage < 50) {
			r = 253;
			g = 204;
			b = 3;
		} else if (percentage < 70) {
			r = 253;
			g = 255;
			b = 71;
		} else if (percentage < 90) {
			r = 170;
			g = 188;
			b = 66;
		} else {
			g = 175;
			r = 100;			
		}
		if (DEBUG) {
			console.log(i + ': ' + percentage + ' (' + numberOfVotes + '): ' + r + ' ' + g);
		}
		if (numberOfVotes != 0) {
			seatList[i].attr({fill: 'rgb(' + r + ',' + g + ',' + b +')'});
		} else {
			seatList[i].attr({fill: DEFAULT_MAP_COLOR});
		}
		textList[i].attr('text', Math.floor(percentage));
		matchDataList[i] = {percentage:percentage, numberOfVotes:numberOfVotes};
	}
	Wt.emit(wListener, 'visualizationCompleted');
}

function setMatchLine(seat) {
	if (matchDataList[seat] != null) {
		matchLine.attr('text', locale.matchStart + ' ' + matchDataList[seat].percentage + '% ' + locale.matchMiddle + ' ' + matchDataList[seat].numberOfVotes + ' ' + locale.matchEnd);
	}
}

function setSeatMapChoiseColor(value) {
	if (value == '0') {
		return 'red';
	} else if (value == '1') {
		return 'green';
	} else if (value == '2') {
		return 'gray';
	} else if (value == '3') { 
		return 'black';
	} else if (value == '-') {
		return 'pink';
	} else {
		console.log('Unkonwn value in setSeatMapChoiseColor: "' + value + '"');
		alert('Unkonwn value in setSeatMapChoiseColor');
		return 'gray';
	}
}

function delaydSetColor(index, color) {
	seatList[index].attr({fill: color});
}

function visualizeVoteChoisesWithMap(data) {
	if (seatMap == null) {
		return ;
	}	
	
	if (seatList.length != data.length) {
		alert("visualizeDataWithMap error: data size missmatch: " + seatList.length + " != " + data.length);
		Wt.emit(wListener, 'visualizationCompleted');
		return 
	}

	var randomOrder = false;
	var startIndex = 0;
	var m = -1;
	matchLine.attr('text', ''); 
	if (Math.random() < 0.5) {
		if (Math.random() < 0.5) {
			startIndex = 0;
			m = 1;	
		} else {
			startIndex = data.length-1;
			m = -1;
		}
	} else {
		randomOrder = true;
	}
	
	for (var i = 0; i < data.length; i++) {
		textList[i].attr('text', '');
		seatList[i].attr({fill: DEFAULT_MAP_COLOR});
		cmd = "delaydSetColor(" + i + ", '" + setSeatMapChoiseColor(data.charAt(i)) + "')";
		
		if (!randomOrder) {
			setTimeout(cmd, 7*(5 + startIndex + i*m));
		} else {
			if (Math.random() < 0.5) {
				setTimeout(cmd, 7*(5 + 0 + i));
			} else {
				setTimeout(cmd, 7*(5 + 200-1 - i));
			}			
		}
		matchDataList[i] = null;
	}
	cmd = "Wt.emit(wListener, 'visualizationCompleted')";
	setTimeout(cmd, 7*(5 + startIndex + 200));
}

function fadeNonSelectedPartymembers(seatBubbles, seatPartyInformation, shownStatus) {
	var statuses = []
	var nonShown = true;
	statuses[0] = true;											//Seat of chair
	for (i = 0; i < shownStatus.length; i++) {
		statuses[shownStatus[i].id] = shownStatus[i].shown;
		if (shownStatus[i].shown) {
			nonShown = false;		
		}
	}
	
	for (i = 0; i < seatPartyInformation.length; i++) {
		if (statuses[parseInt(seatPartyInformation.charAt(i))] || nonShown) {
			seatBubbles[i].attr({opacity: 1.0});
		} else {
			seatBubbles[i].attr({opacity: 0.2});
		}
	}
}

function drawSelectPartyButtons(partyNamesIds, seatMap) {
	if (seatMap == null) {
		return ;
	}
	
	var partyNamesStep = seatMap.width / (partyNamesIds.length + 1);
	for (partyIndex = 0; partyIndex < partyNamesIds.length; partyIndex++) {
		var partyBubble = seatMap.circle(partyNamesStep * (partyIndex + 1), parseInt(BUBBLE_RADIUS * 56), BUBBLE_RADIUS_BIG);
		var text = seatMap.text(partyNamesStep * (partyIndex + 1), parseInt(BUBBLE_RADIUS * 53), partyNamesIds[partyIndex].name);
		text.attr({ "font-size": 16, "font-family": "Arial, Helvetica, sans-serif" });
		partyBubble.attr({fill:"red", cursor: "pointer"});
		partyBubble.node.id = 'party-' + (partyNamesIds[partyIndex].id).toString();
		partyBubble.node.addEventListener('click', handlePartyBubleCliked, false);
		partyButtonsList[partyIndex] = partyBubble;
	}
	partyButtonsList.sort(function(a, b) {return parseInt(a.node.id.substr(6)) - parseInt(b.node.id.substr(6))});
	partyButtonsList.sort();
}

function setPartNames(partyNamesIds) {
	if (DEBUG) {
		console.log('Number of parties set: ' + partyNamesIds.length);
		console.log(partyNamesIds);
	}
	drawSelectPartyButtons(partyNamesIds, seatMap);
	
	for (partyIndex = 0; partyIndex < partyNamesIds.length; partyIndex++) {
		partyNamesIds[partyIndex].shown = false;
	}
	seatMapPartiesShown = partyNamesIds;
}

function setPartyInfo(partySeating) {
	seatsByParty = partySeating;
	if (DEBUG) {
		console.log('Length of party seats data: ' + seatsByParty.length);
		console.log('Data: ' + seatsByParty);
	}
}

function positionRepresentativeBox(seatIndex) {
	var x = seatList[seatIndex].attr('cx');
	var y = seatList[seatIndex].attr('cy');
	var d = document.getElementsByClassName('vaw-representativeInfo')[0];
	var map = $(".vaw-seatmap");
	
	if (d != null && map != null) {
		d.style.left = parseInt(x).toString() + 'px';
		d.style.top = parseInt(y + map.position().top).toString() + 'px';
	}
}

function setLocaleDictionary(dict) {
	if (DEBUG) {
		console.log('setLocaleDictionary: ' + dict);
	}
	locale = dict;
}

function setAdditionalSeats(number, headerForAdditionalSeats) {
	
	if (DEBUG) {
		console.log('setAdditionalSeats: ' + number + ' - ' +  headerForAdditionalSeats)
	}
	
	var additionalSeatsStep = seatMap.width / (number + 1);
	var text = seatMap.text(seatMap.width/2, 47*BUBBLE_RADIUS, headerForAdditionalSeats);
	text.attr({ "font-size": 16, "font-family": "Arial, Helvetica, sans-serif" });
	for (var i = 0; i < number; i++) {
		seatNumber = seatList.length;
		//Copypaste from below, refactor
		seatList[seatNumber] = seatMap.circle(additionalSeatsStep * (i + 1), 50*BUBBLE_RADIUS, BUBBLE_RADIUS);
		seatList[seatNumber].node.addEventListener('click', handleBubleCliked, false);
		seatList[seatNumber].attr({fill: DEFAULT_MAP_COLOR, cursor: "pointer"});   //Set default attributes
		seatList[seatNumber].attr({"stroke-width" : 0});
		seatList[seatNumber].node.id = (seatNumber + 1).toString();
		
		textList[seatNumber] = seatMap.text(seatList[seatNumber].attr('cx'), seatList[seatNumber].attr('cy'), '');
		textList[seatNumber].attr({ "font-size": 16, "font-family": "Arial, Helvetica, sans-serif" });
		if (DEBUG) {
			textList[seatNumber] .attr('text', seatNumber.toString());
		}
	}
}

function drawSeatMap(element, width, height) {
	
	seatMap = Raphael(element, width, height);
	var x0 = width/2;
	var y0 = -50;
	
	var bubbleDistance = 25;
	var ellipseRadiusX = 225;
	var ellipseRadiusY = 125;
	var ellipseRadiusStep = 50;
	var ellipseBubbleAngleStep = 0.01;
	var ellipseBubbleAngleStart = 0.03;
	
	var seatsRows = [{seats:[5, 3, 3, 5]}, {seats:[7, 4, 4, 7]}, {seats:[8, 5, 5, 8]}, {seats:[10, 6, 6, 10]},
		{seats:[8, 7, 7, 8]}, {seats:[6, 8, 8, 7]}, {seats:[4, 9, 9, 4]}, {seats:[0, 9, 9, 0]}];
	var seatsSectionStartingAngles = [Math.PI/10*13, Math.PI/10*14.65, Math.PI/10*15.35, Math.PI/10*17];
	var seatsSectionDirections = [-1, -1, 1, 1];
	
	matchLine = seatMap.text(x0, 20, '');
	matchLine.attr({ "font-size": 16, "font-family": "Arial, Helvetica, sans-serif" });
	
	var seats = [];
	var seatIndex = 0;
	seats[seatIndex] = seatMap.circle(x0, 70, BUBBLE_RADIUS);
	seats[seatIndex].node.id = (seatIndex + 1).toString();
	seatIndex++;
	
	for (var rowIndex = 0; rowIndex < seatsRows.length; rowIndex++) {
		for (var sectionIndex = 0; sectionIndex < seatsSectionStartingAngles.length; sectionIndex++) {
			
			var prevAngle = seatsSectionStartingAngles[sectionIndex] + -seatsSectionDirections[sectionIndex] * BUBBLE_RADIUS/2 * Math.PI/180;
			for (var seatId = 0; seatId <  seatsRows[rowIndex].seats[sectionIndex]; seatId++) {
				var deltta = 0;
				var angle = seatsSectionDirections[sectionIndex] * ellipseBubbleAngleStart;
				var xOld = x0 + ellipseRadiusX * Math.cos(prevAngle);
				var yOld = y0 + ellipseRadiusY * Math.sin(prevAngle);
				var tries = 0;
				while (deltta < bubbleDistance) {
					angle += seatsSectionDirections[sectionIndex] * ellipseBubbleAngleStep;
					var xNew = x0 + ellipseRadiusX * Math.cos(prevAngle + angle);
					var yNew = y0 + ellipseRadiusY * Math.sin(prevAngle + angle);
					deltta = Math.sqrt(Math.pow(xNew - xOld, 2) + Math.pow(yNew - yOld, 2));
					tries++;
				}
				if (DEBUG) {
					console.log("Took " + tries + " tries");
				}
				prevAngle = prevAngle + angle;
				seats[seatIndex] = seatMap.circle(x0 + ellipseRadiusX * Math.cos(prevAngle), -(y0 + ellipseRadiusY * Math.sin(prevAngle)), BUBBLE_RADIUS);
				seats[seatIndex].node.id = (seatIndex + 1).toString();
				seatIndex++;
			}
		}
		ellipseRadiusX += ellipseRadiusStep;
		ellipseRadiusY += ellipseRadiusStep;
		if (DEBUG) {
			seatMap.ellipse(x0, y0, ellipseRadiusX, -y0 + ellipseRadiusY);
		}
	}
	
	for (seatIndex = 0; seatIndex < seats.length; seatIndex++) {
		seats[seatIndex].node.addEventListener('click', handleBubleCliked, false);
		seats[seatIndex].attr({fill: DEFAULT_MAP_COLOR, cursor: "pointer"});         //Set default attributes 
		seats[seatIndex].attr({"stroke-width" : 0});
		
		seatListIndex = parseInt(seats[seatIndex].node.id)-1;
		textList[seatListIndex] = seatMap.text(seats[seatIndex].attr('cx'), seats[seatIndex].attr('cy'), '');
		seatList[seatListIndex] = seats[seatIndex];
		if (DEBUG) {
			textList[seatListIndex].attr('text', seatListIndex.toString());
		}
	}
}

function initVotesAnalyser(listener) {
	var canvas = $(".vaw-seatmap")[0];
	if (canvas == null) {
		alert("Error: seatMap canvas not found");
	} else {
		drawSeatMap(canvas, SEATMAP_WIDTH, SEATMAP_HEIGHT)
	}
    wListener = listener;
}
