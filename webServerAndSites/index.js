function isJSON(str){
    try{
        JSON.parse(str);
        return true;
    } catch(e){
        return false;
    }
}

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

        const messageText = formData.get("message");
        const file = formData.get("image");

        let hasFile = false;
        for(let value of formData.values()){
            if (value instanceof File && value.size > 0){
                hasFile = true;
                break;
            }
        }

        console.log("Form data: ", formData);

        if (hasFile == true){


            const metadata = { message: messageText};

            formData.set("metadata", JSON.stringify(metadata));

            formData.delete("message");


            fetch('/api/chat',{

                method: 'POST',
                body: formData
            })
            .then(response => response.text())
            .then(data =>{
                console.log("Server Response:", data);
                if(isJSON(data)){
                    const res = JSON.parse(data);
                    if(res.filename){
                        const img = document.createElement('img');
                        img.src = '/uploads/' + res.filename;
                        img.alt = "Uploaded Image";
                        img.style.maxWidth = '300px';
                        document.getElementById('divUserDisplay').appendChild(img);
                    }

                    if(res.status === "success" && metadata.message){
                        const userMsgDisplay = document.createElement('div');
                        userMsgDisplay.textContent = metadata.message;
                        document.getElementById('divUserDisplay').appendChild(userMsgDisplay);
                    }

                }

            })
            .catch(error =>{
                console.error("Error:", error);
            });
        } else{
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
        }


    }); 

});