document.addEventListener("DOMContentLoaded", () => {
    const form = document.getElementById("chatMessages");
    const displayDiv = document.getElementById("divUserDisplay");

    // Handle form submission
    form.addEventListener("submit", async (e) => {
        e.preventDefault();

        const formData = new FormData(form);

        try {
            const response = await fetch("/", {
                method: "POST",
                body: formData,
            });

            const result = await response.text();
            console.log(result);

            // After successful upload, fetch latest data
            fetchChatData();
        } catch (error) {
            console.error("Error submitting form:", error);
        }
    });

    // Fetch and display chat data from /chat endpoint
    async function fetchChatData() {
        try {
            const response = await fetch("/chat");
            if (!response.ok) {
                throw new Error("Failed to fetch chat data");
            }

            const data = await response.json();

            displayDiv.innerHTML = `
                <p><strong>Message:</strong> ${sanitize(data.message)}</p>
                <img src="uploads/${encodeURIComponent(data.image)}" style="max-width: 300px;">
            `;
        } catch (error) {
            console.error("Error loading chat data:", error);
            displayDiv.innerHTML = `<p style="color: red;">No chat data found.</p>`;
        }
    }

    // Prevent XSS with simple sanitizer
    function sanitize(str) {
        return str
            .replace(/&/g, "&amp;")
            .replace(/</g, "&lt;")
            .replace(/>/g, "&gt;");
    }

    // Load chat data on initial page load
    fetchChatData();
});
