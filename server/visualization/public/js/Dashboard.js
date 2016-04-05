queue()
    .defer(d3.json, "/api/data")
    .await(makeGraphs);

function makeGraphs(error, apiData) {
    
    //Start Transformations
    var dataSet = apiData;
    dataSet.forEach(function(d) {
	/* date * 1000 because UNIX timestamp is in seconds and Javascript timestamp is in milliseconds */
	d.date = new Date(d.date * 1000);
	d.temperature = +d.temperature;
    });

    //Create a Crossfilter instance
    var ndx = crossfilter(dataSet);

    //Define Dimensions
    var date = ndx.dimension(function(d) {return d.date;});

    //Calculate metrics
    var all = ndx.groupAll();
    var totalTemperature = ndx.groupAll().reduceSum(function(d) {return d.temperature;});
    /* Greetings to this lad : http://jsfiddle.net/mater_tua/QD9Ev/ */
    var temperatureByDate = date.group().reduceSum(function(d) {return d.temperature;});

    //Define threshold values for data
    var minDate = date.bottom(1)[0].date;
    var maxDate = date.top(1)[0].date;

    //Charts
    var heatDateChart = dc.lineChart("#heat-date-chart");
    var averageHeat = dc.numberDisplay("#average-heat");
    var heatDataTable = dc.dataTable("#heat-data-table");
    var heatDataTableSize = dc.numberDisplay("#nb-recordings");

    dc.dataCount("#row-selection")
        .dimension(ndx)
        .group(all);
    
    var numberOfTemperature = dc.numberDisplay("#nb-temperature");
    numberOfTemperature
	.valueAccessor(function(d){return d;})
	.group(all);
    var totalOfTemperature = dc.numberDisplay("#total-temperature")
    totalOfTemperature
	.valueAccessor(function(d){return d;})
	.group(totalTemperature);
    
    averageHeat
	.formatNumber(d3.format(".2f"))
	.group(all)
	.valueAccessor(function(d){
	    var a = totalOfTemperature.value();
	    var b = numberOfTemperature.value();
	    if(b === 0)
		return 0;
	    if(a < 0)
		return 0;
	    return a / b;
	});


    heatDateChart
	.height(220)
	.margins({top: 10, right: 50, bottom: 30, left: 50})
	.dimension(date)
    	.group(temperatureByDate)
	.renderArea(true)
	.transitionDuration(500)
	.x(d3.time.scale().domain([minDate, maxDate]))
	.elasticY(true)
	.renderHorizontalGridLines(true)
    	.renderVerticalGridLines(true)
	.xAxisLabel("Date")
    	.yAxis().ticks(10);    

    heatDataTable
	.dimension(date)
	.group(function(d){return "";})
	.size(500)
	.order(d3.descending)
	.columns([
	    function(d){return d.date;},
	    function(d){return d.temperature;},
	])

    heatDataTableSize
	.formatNumber(d3.format("d"))
	.valueAccessor(function(d){return (function (a, b){return a < b ? a : b;}(numberOfTemperature.value(), heatDataTable.size()));})
	.group(all);
    
    /* Draw the graphs */
    dc.renderAll();
};

