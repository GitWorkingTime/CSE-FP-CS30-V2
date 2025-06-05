document.addEventListener('DOMContentLoaded', function(){
    const form = document.getElementById("chatMessages");
    const divUserDisplay = document.getElementById("divUserDisplay");

    // === HELPER TO DISPLAY MESSAGE/IMAGE ===
    function displayMessageAndImage(data) {
        if (data.message) {
            const userMsgDisplay = document.createElement('div');
            userMsgDisplay.textContent = data.message;
            divUserDisplay.appendChild(userMsgDisplay);
        }

        if (data.image && data.image !== "") {
            const userImg = document.createElement('img');
            userImg.src = '/uploads/' + data.image;
            userImg.alt = "uploaded image";
            userImg.style.maxWidth = '300px';
            userImg.style.display = 'block';
            divUserDisplay.appendChild(userImg);
        }
    }

    // === FETCH INITIAL JSON ON PAGE LOAD ===
    fetch('/uploads/received.json')
    .then(res => res.json())
    .then(data => {
        console.log("Initial JSON fetched:", data);
        displayMessageAndImage(data);
    })
    .catch(err => {
        console.warn("No existing received.json found or error parsing:", err);
    });

    // === SSE LISTENER ===
    const evtSource = new EventSource("/events");

    evtSource.onmessage = function (event) {
        console.log("SSE Event Received:", event.data);
        fetch('/uploads/received.json')
            .then(res => res.json())
            .then(data => {
                console.log("Updated JSON fetched:", data);
                displayMessageAndImage(data);
            })
            .catch(err => {
                console.error("Failed to fetch updated JSON:", err);
            });
    };

    // === FORM SUBMISSION HANDLER ===
    form.addEventListener('submit', function (event) {
        event.preventDefault();
        console.log("Form submission intercepted.");

        const formData = new FormData(this);
        const messageText = formData.get("message");

        const metadata = { message: messageText };
        formData.set("metadata", JSON.stringify(metadata));
        formData.delete("message");

        fetch('/api/chat', {
            method: 'POST',
            body: formData
        })
        .then(response => response.text())
        .then(data => {
            console.log("Server Response:", data);
            form.reset(); // UI update will now happen via SSE
        })
        .catch(error => {
            console.error("Upload error:", error);
            form.reset();
        });
    });
});









// function isJSON(str){
//     try{
//         JSON.parse(str);
//         return true;
//     } catch(e){
//         return false;
//     }
// }

// /*
// Because this file is called in the <head> portion of the html file, we will use
// 'DOMContentLoaded' event to ensure that all HTML and associated files are loaded
// before we do any operations 
// */
// document.addEventListener('DOMContentLoaded', function(){
//     document.getElementById("chatMessages").addEventListener('submit', function(event){
//         //Stop the form from submitting normally
//         event.preventDefault(); 

//         const formData = new FormData(this);
//         const jsonObj = {};

//         const messageText = formData.get("message");
//         const file = formData.get("image");

//         let hasFile = false;
//         for(let value of formData.values()){
//             if (value instanceof File && value.size > 0){
//                 hasFile = true;
//                 break;
//             }
//         }

//         console.log("Form data: ", formData);

//         if (hasFile == true){


//             const metadata = { message: messageText};

//             formData.set("metadata", JSON.stringify(metadata));

//             formData.delete("message");


//             fetch('/api/chat',{

//                 method: 'POST',
//                 body: formData
//             })
//             .then(response => response.text())
//             .then(data =>{
//                 console.log("Server Response:", data);
//                 if(isJSON(data)){
//                     const res = JSON.parse(data);
//                     if(res.filename){
//                         const img = document.createElement('img');
//                         img.src = '/uploads/' + res.filename;
//                         img.alt = "Uploaded Image";
//                         img.style.maxWidth = '300px';
//                         document.getElementById('divUserDisplay').appendChild(img);
//                     }

//                     if(res.status === "success" && metadata.message){
//                         const userMsgDisplay = document.createElement('div');
//                         userMsgDisplay.textContent = metadata.message;
//                         document.getElementById('divUserDisplay').appendChild(userMsgDisplay);
//                     }

//                 }
//                 this.reset();

//             })
//             .catch(error =>{
//                 console.error("Error:", error);
//                 this.reset();
//             });
//         } else{
//             //Copies all key-value pairs from the FormData and adds it to a regular JS object
//             formData.forEach((value, key) =>{
//                 jsonObj[key] = value;
//             });

//             const jsonString = JSON.stringify(jsonObj);
//             console.log(jsonString);

//             // const userMsg = jsonObj.message;
//             // const displayDiv = document.getElementById('divUserDisplay');

//             // const userMsgDisplay = document.createElement('div');
//             // userMsgDisplay.textContent = userMsg;
//             // displayDiv.appendChild(userMsgDisplay);


//             fetch('/api/chat',{

//                 method: 'POST',
//                 headers: {
//                     'Content-Type': 'application/json'
//                 },
//                 body: jsonString
//             })
//             .then(response => response.text())
//             .then(data =>{
//                 console.log("Server Response:", data);
//                 this.reset();
//             })
//             .catch(error =>{
//                 console.error("Error:", error);
//                 this.reset();
//             });
//         }


//     }); 

//     //SSE client-side connection for receiving updates
//     const eventSource = new EventSource("/events");

//     eventSource.onopen = function(event){
//         console.log("SSE Connection opened");
//     };

//     eventSource.onmessage = function(event) {
//         console.log("Received SSE message:", event.data);
        
//         // If the server sends a JSON message
//         if (isJSON(event.data)) {
//             const data = JSON.parse(event.data);

//             // Handle the received data, e.g., display a new message
//             const displayDiv = document.getElementById('divUserDisplay');
//             const newMsgDisplay = document.createElement('div');
//             newMsgDisplay.textContent = data.message;  // Assuming message is part of the data
//             displayDiv.appendChild(newMsgDisplay);

//             // Handle any other logic with the incoming data
//             if (data.filename) {
//                 const img = document.createElement('img');
//                 img.src = '/uploads/' + data.filename;
//                 img.alt = "Uploaded Image";
//                 img.style.maxWidth = '300px';
//                 displayDiv.appendChild(img);
//             }
//         }
//     };

//     eventSource.onerror = function(event) {
//         console.error("SSE Error:", event);
//         // Handle error scenarios, like reconnecting if necessary
//     };


// });