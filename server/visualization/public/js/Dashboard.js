queue()
    .defer(d3.json, "/api/data")
    .await(makeGraphs);

function makeGraphs(error, apiData) {
    
    //Start Transformations
    var dataSet = apiData;
    var dateFormat = d3.time.format("%m/%d/%Y");
    dataSet.forEach(function(d) {
	console.log(d.date);
	d.date = dateFormat.parse(d.date);
	d.date.setDate(1);
	console.log(d.date);
	d.temperature = +d.temperature;
    });

    //Create a Crossfilter instance
    var ndx = crossfilter(dataSet);

    //Define Dimensions
    var datePosted = ndx.dimension(function(d) { return d.date; });
    var totalDonations  = ndx.dimension(function(d) { return d.temperature; });


    //Calculate metrics
    var projectsByDate = datePosted.group(); 
    var all = ndx.groupAll();

    var netTotalDonations = ndx.groupAll().reduceSum(function(d) {return d.temperature;});

    //Define threshold values for data
    var minDate = datePosted.bottom(1)[0].date;
    var maxDate = datePosted.top(1)[0].date;

    //Charts
    var dateChart = dc.lineChart("#date-chart");
    var averageHeat = dc.numberDisplay("#average-heat");
    var dataTable = dc.dataTable("#data-table");
    var dataTableSize = dc.numberDisplay("#nb-recordings");

    dc.dataCount("#row-selection")
        .dimension(ndx)
        .group(all);
    
    var totalProjects = dc.numberDisplay("#total-projects");
    totalProjects
	.valueAccessor(function(d){return d; })
	.group(all);
    var netDonations = dc.numberDisplay("#net-donations")
    netDonations
	.valueAccessor(function(d){return d; })
	.group(netTotalDonations);
    
    averageHeat
	.formatNumber(d3.format(".2f"))
	.group(all)
	.valueAccessor(function(d){
	    var a = netDonations.value();
	    var b = totalProjects.value();
	    if(b === 0)
		return 0;
	    if(a < 0)
		return 0;
	    return a / b;
	});

    dateChart
	.height(220)
	.margins({top: 10, right: 50, bottom: 30, left: 50})
	.dimension(datePosted)
	.group(projectsByDate)
	.renderArea(true)
	.transitionDuration(500)
	.x(d3.time.scale().domain([minDate, maxDate]))
	.elasticY(true)
	.renderHorizontalGridLines(true)
    	.renderVerticalGridLines(true)
	.xAxisLabel("Year")
	.yAxis().ticks(6);

    dataTable
	.dimension(datePosted)
	.group(function(d){return "";})
	.size(500)
	.order(d3.descending)
	.columns([
	    function(d){return d.date;},
	    function(d){return d.temperature;},
	])

    dataTableSize
	.formatNumber(d3.format("d"))
	.valueAccessor(function(d){return (function (a, b){return a < b ? a : b;}(totalProjects.value(), dataTable.size()));})
	.group(all);
    
    /* Draw the graphs */
    dc.renderAll();
};

