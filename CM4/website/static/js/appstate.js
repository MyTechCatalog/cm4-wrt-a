appstate_subscribers = []

subscribe_to_appstate = function(f) {
    appstate_subscribers.push(f);
}

$(function(){
    appstate = {}
    const source = new EventSource("/stream");
    source.onmessage = function(msg) {
        appstate = JSON.parse(msg.data)
        for (s of appstate_subscribers) {
            s(appstate)
        }
    }

    source.onopen = function() {        
        console.log('SSE /stream opened');
    }

    source.onerror = (err) => {
        console.error("EventSource failed:", err);
    };
});
