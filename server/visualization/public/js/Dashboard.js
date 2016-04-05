queue()
    .defer(d3.json, "/api/data")
    .await(makeGraphs);

function makeGraphs(error, apiData) {
    
    //Start Transformations
    var dataSet = apiData;
//    var dateFormat = d3.time.format("%Y/%m/%d");
//    var dateFormat = d3.time.format("%m/%d/%Y");
    dataSet.forEach(function(d) {
	d.date = new Date(d.date * 1000);
//	console.log(d.date);
//	d.date = dateFormat.parse(d.date);
//	d.date.setDate(1);
//	console.log(d.date);
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

    var temperature = totalDonations.group();

    var netTotalDonations = ndx.groupAll().reduceSum(function(d) {return d.temperature;});
    /* Greetings to this lad : http://jsfiddle.net/mater_tua/QD9Ev/ */
    var temperatureByDate = datePosted.group().reduceSum(function(d) {return d.temperature;});

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


    /*
    var data = [
        {"time":"2014-06-09T18:45:00.000Z","input":17755156,"output":250613233.333333},
        {"time":"2014-06-09T18:46:00.000Z","input":18780286.6666667,"output":134619822.666667},
        {"time":"2014-06-09T18:47:00.000Z","input":20074614.6666667,"output":203239834.666667},
        {"time":"2014-06-09T18:48:00.000Z","input":22955373.3333333,"output":348996205.333333},
        {"time":"2014-06-09T18:49:00.000Z","input":19119089.3333333,"output":562631022.666667},
        {"time":"2014-06-09T18:50:00.000Z","input":15404272,"output":389916332},
        {"time":"2014-06-09T18:51:00.000Z","input":null,"output":null},
        {"time":"2014-06-09T21:25:20.000Z","input":5266038.66666667,"output":62598396},
        {"time":"2014-06-09T21:26:20.000Z","input":6367678.66666667,"output":84494096},
        {"time":"2014-06-09T21:27:20.000Z","input":5051610.66666667,"output":88812540},
        {"time":"2014-06-09T21:28:20.000Z","input":5761069.33333333,"output":79098036},
        {"time":"2014-06-09T21:29:20.000Z", "input":5110277.33333333,"output":45816729.3333333}
    ];
    var xf = crossfilter(data);
    var time   = xf.dimension(function(d){ 
        return new Date( d.time ); 
    });
    var input  = time.group().reduceSum(function(d){ 
        return d.input  
    });
    var output = time.group().reduceSum(function(d){ 
        return d.output 
    });

    var chart = dateChart;

    var width = 600;
    chart.height(300)
        //.elasticY(true)
        .renderHorizontalGridLines(true)
        .legend(dc.legend().x((width*0.75)).y(0).itemHeight(10).gap(5))
        .margins({top: 20, right: 40, bottom: 25, left: (width * 0.08)})
        .brushOn(false)
        .renderArea(false)
        .width(width)
        .dimension(time)
        .group(input, "Input Bits")
        .defined(function(d){
            return (d.data.value !== null);
        })
        .renderDataPoints({radius: 0.5, fillOpacity: 0.8, strokeOpacity: 0.8})
	.x(d3.time.scale().domain(d3.extent(data, function(d){
            return new Date( d.time );   
	})));

    chart.stack(output, "Output bits");
    chart.yAxis().tickFormat(function(d){
        if(d === 0){
            return 0;
        }
        return d3.format("4s")(d);
    });
    */
    var toto = ndx.dimension(function(d){return d.date;});
    var input = toto.group().reduceSum(function(d){return d.temperature;});
    
    dateChart
	.height(220)
	.margins({top: 10, right: 50, bottom: 30, left: 50})
	.dimension(datePosted)
    	.group(temperatureByDate)
	.renderArea(true)
	.transitionDuration(500)
	.x(d3.time.scale().domain([minDate, maxDate]))
	.elasticY(true)
	.renderHorizontalGridLines(true)
    	.renderVerticalGridLines(true)
	.xAxisLabel("Year")
    	.yAxis().ticks(10);
    
    /*
    dateChart
	.height(220)
	.margins({top: 10, right: 50, bottom: 30, left: 50})
	.dimension(datePosted)
    	.group(projectsByDate)
//	.valueAccessor(function(d){return d.temperature;})
	.renderArea(true)
	.transitionDuration(500)
	.x(d3.time.scale().domain([minDate, maxDate]))
	.elasticY(true)
	.renderHorizontalGridLines(true)
    	.renderVerticalGridLines(true)
	.xAxisLabel("Year")
    	.yAxis().ticks(10);
*/
    /*
    dateChart
	.height(220)
	.margins({top: 10, right: 50, bottom: 30, left: 50})
	.dimension(totalDonations)
	.group(temperature)
	.x(d3.domain([0, 100]));
    */
/*
    var data = [{"user":"jim","scores":[40,20,30,24,18,40]},
		{"user":"ray","scores":[24,20,30,41,12,34]}];


    var chart = d3.select("#date-chart").append("svg")           
        .data(data)
        .attr("class","chart")
        .attr("width",800)
        .attr("height",350);


    chart.selectAll("rect")    
	.data(function(d){return d3.values(d.scores);})    
	.enter().append("rect")
	.attr("y", function(d,i){return i * 20;})
	.attr("width",function(d){return d;})
	.attr("height", 20);
    */

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

