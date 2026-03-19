import requests
import smtplib
from email.mime.text import MIMEText
from email.mime.multipart import MIMEMultipart
import os
import json

# --- CONFIGURATION ---
# Replace these with your actual values or set them in your environment
BACKEND_URL = "https://cosmic-rays-backend.evan-t-beechem.workers.dev"
ADMIN_API_KEY = os.getenv("ADMIN_API_KEY", "your_secret_admin_key")

# SMTP Configuration (Example: Gmail)
SMTP_SERVER = "smtp.gmail.com"
SMTP_PORT = 587
SENDER_EMAIL = os.getenv("SENDER_EMAIL", "your_email@gmail.com")
SENDER_PASSWORD = os.getenv("SENDER_PASSWORD", "your_app_password")

def get_subscribers():
    """Fetches the list of subscribers from the Cloudflare Worker."""
    headers = {"Authorization": f"Bearer {ADMIN_API_KEY}"}
    try:
        response = requests.get(f"{BACKEND_URL}/emails", headers=headers)
        response.raise_for_status()
        return response.json()
    except Exception as e:
        print(f"Error fetching subscribers: {e}")
        return []

def send_bulk_email(subject, body, subscribers):
    """Sends a bulk email to all subscribers."""
    if not SENDER_EMAIL or not SENDER_PASSWORD:
        print("SENDER_EMAIL and SENDER_PASSWORD must be configured.")
        return

    try:
        # Create a connection
        server = smtplib.SMTP(SMTP_SERVER, SMTP_PORT)
        server.starttls()
        server.login(SENDER_EMAIL, SENDER_PASSWORD)

        for subscriber in subscribers:
            recipient = subscriber['email']
            print(f"Sending email to: {recipient}")

            msg = MIMEMultipart()
            msg['From'] = SENDER_EMAIL
            msg['To'] = recipient
            msg['Subject'] = subject

            msg.attach(MIMEText(body, 'html'))
            server.send_message(msg)

        server.quit()
        print("Bulk email operation complete.")
    except Exception as e:
        print(f"Error sending emails: {e}")

def main():
    print("--- Cosmic Rays Email Manager ---")
    subscribers = get_subscribers()
    
    if not subscribers:
        print("No subscribers found.")
        return

    print(f"Found {len(subscribers)} subscribers.")
    
    action = input("Actions: (L)ist all, (S)end bulk email, (Q)uit: ").lower()
    
    if action == 'l':
        for s in subscribers:
            print(f"- {s['email']} (Joined: {s['created_at']})")
    
    elif action == 's':
        subject = input("Enter Subject: ")
        print("Enter Body (Type 'DONE' on a new line to finish):")
        body_lines = []
        while True:
            line = input()
            if line == "DONE":
                break
            body_lines.append(line)
        body = "\n".join(body_lines)
        
        confirm = input(f"Send this email to {len(subscribers)} people? (y/n): ")
        if confirm.lower() == 'y':
            send_bulk_email(subject, body, subscribers)
        else:
            print("Cancelled.")

if __name__ == "__main__":
    main()
