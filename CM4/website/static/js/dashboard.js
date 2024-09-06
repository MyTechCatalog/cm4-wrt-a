$(function () {

    // Inhibit form submit
    $('form').submit(function(e){
        e.preventDefault();
        console.log("form submit:", $(this).serialize() );
        return false;
    });
   
    $('form#watchdog-form input').change(async function(e){
        e.preventDefault();                   
        $('#loader-parent').fadeIn();
        var form_data = { timeout_sec: parseInt($("#watchdog-timeout").val()),
                        max_retries: parseInt($("#watchdog-max_retries").val()),
                        is_enabled: $("#watchdog-enable").is(":checked")};
        
        console.log("form input change:", this.name, $(this).val() );
        post_api_request("/api/settings/watchdog", form_data, function(response){
            console.log("/api/settings/watchdog post response:", response);
            if ("error" in response) {
                display_error(response.error);
            }
        });        
        $('#loader-parent').fadeOut();        
        return false;
    });

    $("#fan-pwm-opt").on("change", function() {
        $('#fan-pwm-input').val($(this).val());
    });

    $('#fan-pwm-input').change(function(){
        $('#loader-parent').fadeIn();
        let name = $(this).attr("fan-id");
        let data = {fan_name: name, fan_pwm_pct: parseInt($(this).val())};
        post_api_request("/api/settings/fan_pwm", data, function(response){
            console.log("/api/settings/fan_pwm post response:", response);
            if ("error" in response) {
                display_error(response.error);
            } else {
                let e = document.getElementById("fan-pwm-opt");
                e.options[e.selectedIndex].value = response[name];
            }
        });
        $('#loader-parent').fadeOut();
    });
});

lastMessage = "";

function display_error(message){
    if(message){
        if (message != lastMessage) {
            lastMessage = message;
            $('#status-message').text(message);
            document.getElementById('status-panel').style.display='block';
            $("html, body").animate({ scrollTop: $(document).height()-$(window).height() });
        }        
    } else {
        document.getElementById('status-panel').style.display='none';
    }    
}

function update_from_appstate(appstate) {
    plot_temperature(appstate);
    plot_fan_rpm(appstate);
}

window.subscribe_to_appstate(update_from_appstate); // Subscribe to appstate updates

function plot_default_chart(chartID, titleText){
    var graphs = null;
    var layout = { title: {
                        text: titleText,
                        font: {
                            family: 'Courier New, monospace',
                            size: 32
                        },
                        xref: 'paper',
                        automargin: true,
                    }, margin: {
                        t: 20, //top margin
                        l: 60, //left margin
                        r: 20, //right margin
                        b: 50 //bottom margin
                    }, xaxis: { title: {
                        font: { family: 'Courier New, monospace', size: 20, color: '#7f7f7f' } }
                    }, yaxis: { title: {
                        font: { family: 'Courier New, monospace', size: 20, color: '#7f7f7f' } }
                    }};
    
    var config = {responsive: true};

    layout["annotations"] = [
        {   
            text: "No Data.",
            xref: "paper",
            yref: "paper",
            showarrow: false,
            font: {
                size: 28
            }
        }
    ]

    layout.plot_bgcolor = "rgba(0,0,0,0)";

    if(window.matchMedia('(prefers-color-scheme: dark)').matches){
        layout.paper_bgcolor = "LightGrey";
    }

    Plotly.newPlot(chartID, graphs, layout, config);    
}

function plot_temperature(aDict) {
    var graphs = [];
    Object.entries(aDict["temperature_c"]).forEach(([key, val]) => {
        // Create a new JavaScript Date object based on the timestamp
        // multiplied by 1000 so that the argument is in milliseconds, as opposed to seconds.
        let trace = {name: key,	type: 'scatter', mode: 'lines', x: aDict["timestamp_sec"].map((x) => new Date(x * 1000)),	y: val }; 
        graphs.push(trace);
    });    
    
    var layout = { margin: {
                        t: 0, //top margin
                        l: 50, //left margin
                        r: 20, //right margin
                        b: 50 //bottom margin
                    }, yaxis: { rangemode: "tozero"
                        , title: {                        
                            text: "Temperature (\u00B0C)", 
                            font: { family: 'Courier New, monospace', size: 16, color: '#7f7f7f' } 
                        }
                    }, xaxis: { type: "date"
                        , title: {
                            /*text: "Channel",*/
                            font: { family: 'Courier New, monospace', size: 16, color: '#7f7f7f' } 
                        }
                        , tickfont: { family: 'Courier New, monospace', size: 16, color: '#7f7f7f' }
                    }, plot_bgcolor: "rgba(0,0,0,0)"};
    
    if (window.matchMedia('(prefers-color-scheme: dark)').matches){
        layout.paper_bgcolor = "LightGrey";
    }
    var config = {responsive: true}
    Plotly.newPlot('temperature-chart', graphs, layout, config);
}

function plot_fan_rpm(aDict) {
    var graphs = [];
    Object.entries(aDict["tachometer_rpm"]).forEach(([key, val]) => {
        // Create a new JavaScript Date object based on the timestamp
        // multiplied by 1000 so that the argument is in milliseconds, as opposed to seconds.
        let trace = {name: key,	type: 'scatter', mode: 'lines', x: aDict["timestamp_sec"].map((x) => new Date(x * 1000)),	y: val }; 
        graphs.push(trace);
    });    
    
    var layout = { margin: {
                        t: 0, //top margin
                        l: 50, //left margin
                        r: 20, //right margin
                        b: 50 //bottom margin
                    }, yaxis: { rangemode: "tozero"
                        , title: {                        
                            text: "Revolutions Per Minute (RPM)", 
                            font: { family: 'Courier New, monospace', size: 16, color: '#7f7f7f' } 
                        }
                    }, xaxis: { type: "date"
                        , title: {
                            /*text: "Channel",*/
                            font: { family: 'Courier New, monospace', size: 16, color: '#7f7f7f' } 
                        }
                        , tickfont: { family: 'Courier New, monospace', size: 16, color: '#7f7f7f' }
                    }, plot_bgcolor: "rgba(0,0,0,0)"};    

    if (window.matchMedia('(prefers-color-scheme: dark)').matches){
        layout.paper_bgcolor = "LightGrey";
    }
    var config = {responsive: true}
    Plotly.newPlot('fan-rpm-chart', graphs, layout, config);
}

function post_api_request(theUrl, aDict, successCallback){
    $.ajax({
        type: 'POST',
        url: theUrl,
        processData: false,            
        contentType: 'application/json',
        data: JSON.stringify(aDict),
        success: function (response) {
            if (successCallback) {
                successCallback(response);
            } else {
                console.log(response);
            }
        },
        error: function (err) {
            console.log(err);
            display_error(err.statusText);
        }
    });
}