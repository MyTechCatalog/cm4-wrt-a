$(function(){
    document.getElementById('quit').style.display='block';

    $('#quit button[name=yes]').click(function(e){
        
        $.ajax({
            url : "/quit/now",
            type: "post",
        }).done(function(response){
            console.log("/quit/now response:", response);
            $('#bye').show();
            document.getElementById('quit').style.display='none';
        });
    })

    $('#quit button[name=no]').click(function(e){
        history.back();
    })
});
