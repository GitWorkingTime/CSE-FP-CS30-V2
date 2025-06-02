/*
Because this file is called in the <head> portion of the html file, we will use
'DOMContentLoaded' event to ensure that all HTML and associated files are loaded
before we do any operations 
*/
document.addEventListener('DOMContentLoaded', function(){
    document.getElementById("chatMessages").addEventListener('submit', function(event){
        //Stop the form from submitting normally
        event.preventDefault(); 

        const formData = new FormData(this);
        const jsonObj = {};

        //Copies all key-value pairs from the FormData and adds it to a regular JS object
        formData.forEach((value, key) =>{
            jsonObj[key] = value;
        });

        const jsonString = JSON.stringify(jsonObj);
        console.log(jsonString);

        const userMsg = jsonObj.message;
        const displayDiv = document.getElementById('divUserDisplay');

        const userMsgDisplay = document.createElement('div');
        userMsgDisplay.textContent = userMsg;
        displayDiv.appendChild(userMsgDisplay);

        fetch('/api/chat',{

            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: jsonString
        })
        .then(response => response.text())
        .then(data =>{
            console.log("Server Response:", data);
        })
        .catch(error =>{
            console.error("Error:", error);
        })

    }); 

});