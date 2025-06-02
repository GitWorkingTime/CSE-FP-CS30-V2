document.addEventListener("DOMContentLoaded", function () {
	displayChatMessage();
  document.getElementById("chatMessages").addEventListener("submit", function (event) {
    event.preventDefault();
    handleFormSubmission(event);
  });
});

function handleFormSubmission(event){
	const form = event.target;
	const formData = new FormData(form);
    const jsonObject = {};

    fetch("/api", {

    	method: "POST",
    	body: formData

    })
    .then(response => response.text())
    .then(data => {
    	console.log("Server responded with:", data);
    	displayChatMessage();
    })
    .catch(error => {
    	console.error("Error sending form:", error);
    	alert("Failed to send form!");
    })

}

function displayChatMessage(){
	fetch('/received.json')
	  .then(response => {
	    if (!response.ok) {
	      throw new Error('Network response was not OK');
	    }
	    return response.json();
	  })
	  .then(data => {
	  	const chatContainer = document.getElementById("divUserDisplay");

	  	if(data.message){
	  		const divChatText = document.createElement("div");
	  		divChatText.classList.add("divChatText");
	  		divChatText.textContent = data.message;
	  		chatContainer.appendChild(divChatText);
	  	}

	  	if(data.image){
	  		const img = document.createElement("img");
	  		img.src = "/" + data.image;
	  		img.alt = "User uploaded image";
	  		img.style.maxWidth = "200px";
	  		chatContainer.appendChild(img);
	  	}
	  })
	  .catch(error => {
		console.error('Error fetching JSON:', error);
        document.getElementById('chat').textContent = 'Failed to load chat message.';
	  });

}
