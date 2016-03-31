queue()
    .defer(d3.json, "/api/data")
    .await(makeGraphs);

function makeGraphs(error, apiData) {
    
    //Start Transformations
    var dataSet = apiData;
    var dateFormat = d3.time.format("%m/%d/%Y");
    dataSet.forEach(function(d) {
	d.date_posted = dateFormat.parse(d.date_posted);
	d.date_posted.setDate(1);
	d.total_donations = +d.total_donations;
    });

    //Create a Crossfilter instance
    var ndx = crossfilter(dataSet);

    //Define Dimensions
    var datePosted = ndx.dimension(function(d) { return d.date_posted; });
    var totalDonations  = ndx.dimension(function(d) { return d.total_donations; });


    //Calculate metrics
    var projectsByDate = datePosted.group(); 
    var all = ndx.groupAll();

    //Calculate Groups

    //	var totalHeat = ndx.groupAll().reduceSum(function(d) {return d.total_donations;});
    /*
      var totalDonationsState = state.group().reduceSum(function(d) {
      return d.total_donations;
      });

      var totalDonationsGrade = gradeLevel.group().reduceSum(function(d) {
      return d.grade_level;
      });

      var totalDonationsFundingStatus = fundingStatus.group().reduceSum(function(d) {
      return d.funding_status;
      });

*/
      var netTotalDonations = ndx.groupAll().reduceSum(function(d) {return d.total_donations;});
    /*
    var netTotalDonations = datePosted.group().reduce(reduceAddVal, reduceDeleteVal, initialValue);
    */
    //Define threshold values for data
    var minDate = datePosted.bottom(1)[0].date_posted;
    var maxDate = datePosted.top(1)[0].date_posted;

    /*
    function reduceAddVal(p, v){
	p.count++;
	p.total += v.total_donations;
	p.average = p.total / p.count;
	return p;
    }
    function reduceDeleteVal(p, v){
	p.count--;
	p.total -= v.total_donations;
	p.average = p.total / p.count;
	return p;
    }
    function initialValue(){
	return {
	    count:0,
	    total:0,
	    average:0
	};
    }
*/

    //Charts
    var dateChart = dc.lineChart("#date-chart");
    var averageHeat = dc.numberDisplay("#average-heat");
    var dataTable = dc.dataTable("#data-table");

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
	    return totalProjects.value() === 0 ? 0 : netDonations.value() < 0 ? 0 : netDonations.value() / totalProjects.value();
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
	.columns([
	    function(d){return d.id;},
	    function(p){return p.count;},
	    function(d){return d.date_posted;},
	    function(d){return d.total_donations;},
	])
    
    /* Draw the graphs */
    dc.renderAll();
};

