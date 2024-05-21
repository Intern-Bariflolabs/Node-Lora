# Import necessary modules
import smtplib
from email.mime.text import MIMEText
from email.mime.multipart import MIMEMultipart

# Define the SendMail class
class SendMail:
    def __init__(self):
        self.__SMTP_SERVER = 'smtp.gmail.com'
        self.__SMTP_PORT = 587
        self.__SENDER_MAIL = 'care.bariflolabs@gmail.com'
        self.__EMAIL_HOST_PASSWORD = 'lhbvxthqobhnztrn'

    def send_email(self, receiver_email, subject, template_path, substitutions):
        try:
            # Read the HTML template file
            with open(template_path, 'r') as file:
                html_content = file.read()

            # Perform substitutions in the HTML content
            for key, value in substitutions.items():
                html_content = html_content.replace('{{ ' + key + ' }}', str(value))

            # Create a MIMEText object for the email body
            msg = MIMEMultipart()
            msg['From'] = self.__SENDER_MAIL
            msg['To'] = receiver_email
            msg['Subject'] = subject
            msg.attach(MIMEText(html_content, 'html'))

            # Create an SMTP session
            server = smtplib.SMTP(self.__SMTP_SERVER, self.__SMTP_PORT)
            server.starttls()
            server.login(self.__SENDER_MAIL, self.__EMAIL_HOST_PASSWORD)
            text = msg.as_string()
            server.sendmail(self.__SENDER_MAIL, receiver_email, text)
            print('\033[92mEmail sent successfully!\033[0m')
        except Exception as e:
            print(f'Error sending email: {e}')
        finally:
            server.quit()  # Close the SMTP session

# Create an instance of the SendMail class
S = SendMail()

# Prepare email data
args = (
    'satyajit.bariflo@outlook.com',  # Receiver's email
    'New Current Status',  # Email subject
    'email_template.html',  # HTML template file path
    {'current_b': 100, 'current_y': 200, 'current_r': 300}  # Substitutions dictionary
)

# Send the email using the send_email method
S.send_email(*args)
