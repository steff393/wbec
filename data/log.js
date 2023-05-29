// Copyright (c) 2023 steff393, MIT license

window.addEventListener('DOMContentLoaded', () => {
	let tableHeaders = document.querySelectorAll('.table-head');
	var sortOrder = 1;

	document.getElementById('btnLog'). addEventListener('click', function() {window.location.href = "/log.html"});
	document.getElementById('btnCfg'). addEventListener('click', function() {window.location.href = "/cfg.html"});
	document.getElementById('btnJson').addEventListener('click', function() {window.location.href = "/json"});
	document.getElementById('btnEdit').addEventListener('click', function() {window.location.href = "/edit"});
	document.getElementById('btnUpd'). addEventListener('click', function() {window.location.href = "/update"});
	document.getElementById('btnExit').addEventListener('click', function() {window.location.href = "/"});

	function init() {
		for (const element of tableHeaders) {
			element.addEventListener('click', sortTable);
		}
		const cfgCntWb = 1; // number of connected wallboxes
		for (let i = 0; i < cfgCntWb; i++) {
			let url="/chargelog?id=" + i + "&len=10";
			fetch(url)
			.then(response => response.json())
			.then((msg) => updateData(msg))
		}
	}


	function formatDate(xtime) {  // convert unixtime to formatted date
		const date = new Date(xtime * 1000);
		const day     =  date.getDate().toString().padStart(2, '0'); // day with leading 0
		const month   = (date.getMonth() + 1).toString().padStart(2, '0'); // month with leading 0 (months start at 0)
		const year    =  date.getFullYear().toString();
		const hours   =  date.getHours().toString().padStart(2, '0'); 
		const minutes =  date.getMinutes().toString().padStart(2, '0');
		const formattedDate = `${day}.${month}.${year} | ${hours}:${minutes}`;
		return(formattedDate);
	}


	function formatDur(dur) {
		const hours   = Math.floor(dur / 3600);
		const minutes = Math.floor((dur % 3600) / 60).toString().padStart(2, '0');
		const formattedDate = `${hours}:${minutes}`;
		return(formattedDate);
	}
	

	function updateData(data) {
		const message = data;
		let header = document.getElementById('logTable');
		for (const line of message.line.reverse()) {
			var row = document.createElement('div');
			row.className = 'table-row';
			var cells = [];
			for (var i = 0; i <= 7; i++) {         // nr of cells, incl. hidden
				var cell = document.createElement('div');
				cell.className = 'table-cell';
				row.appendChild(cell);
				cells.push(cell);
			}
			header.appendChild(row);
			cells[0].innerHTML = formatDate(line.timestamp);
			cells[1].innerHTML =            line.timestamp;
			cells[1].style.display = 'none'; // only for sorting
			cells[2].innerHTML = formatDate(line.timestamp + line.duration);
			cells[3].innerHTML =            line.timestamp + line.duration
			cells[3].style.display = 'none'; // only for sorting
			cells[4].innerHTML =           (line.energy / 1000).toFixed(3);
			cells[5].innerHTML = formatDur (line.duration);
			cells[6].innerHTML =            line.duration;
			cells[6].style.display = 'none'; // only for sorting
			cells[7].innerHTML =            line.box + 1;
		}
	}


	function sortTable() {
		const col = Array.from(this.parentNode.children).indexOf(this);
		const map = {0:1, 1:3, 2:4, 3:6, 4:7};  // map the visible clicked column (of header) to the maybe invisible columns, see above     
		var table = document.getElementById('logTable');
		var rows  = table.querySelectorAll('.table-row');
		var sortedRows = Array.prototype.slice.call(rows, 1); // slice(1) removes the header line
		sortedRows.sort(function(row1, row2) {
			var val1 = parseFloat(row1.querySelectorAll('.table-cell')[map[col]].textContent);
			var val2 = parseFloat(row2.querySelectorAll('.table-cell')[map[col]].textContent);
			if (val1 < val2) {
				return -1 * sortOrder;
			}
			if (val1 > val2) {
				return 1  * sortOrder;
			}
			return 0;
		});
		table.innerHTML = "";          // delete the content
		table.appendChild(rows[0]);    // add the header again
		sortedRows.forEach(function(row) {
			table.appendChild(row);      // append sorted rows
		});
		sortOrder *= -1;               // invert the sortOrder
	}
	
	
	init();

});


function exportToExcel() {
	var table = document.getElementById("logTable");
	var rows = table.getElementsByClassName("table-row");
	var csvContent = '"Startdatum","Startzeit","Enddatum","Endzeit","Energie","Dauer","Wallbox"';

	for (var i = 0; i < rows.length; i++) {
		var cells = rows[i].getElementsByClassName("table-cell");

		for (var j = 0; j < cells.length; j++) {
			var cellData = cells[j].innerText.trim();
			if (j!=1 && j!=3 && j!=6) {  // remove the hidden colums with raw values
				if (j==4) {
					csvContent += '"' + cellData.replace('.', ',') + '",';  // special handling for the energy decimal point
				} else 
				{
					csvContent += '"' + cellData.replace(' | ', '","') + '",';
				}
			}
		}
		csvContent += "\n";
	}

	// create a temporary link in order to download the csv
	var downloadLink = document.createElement("a");
	downloadLink.href = "data:text/csv;charset=utf-8," + encodeURIComponent(csvContent);
	downloadLink.download = "wbecLog.csv";
	downloadLink.style.display = "none";
	document.body.appendChild(downloadLink);
	downloadLink.click();
	document.body.removeChild(downloadLink);
}
