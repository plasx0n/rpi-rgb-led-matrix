import time
import requests
import json
import datetime
from requests.adapters import HTTPAdapter
from urllib3.util.retry import Retry



file = "home/pi/rpi-rgb-led-matrix/openweather/weather.txt"
URL="http://api.openweathermap.org/data/2.5/weather?lat=44.800&lon=-0.617&appid=1554f8b227d5a6f2245d99740b61dfdd&units=metric"

# robust get since errors after a few calls 
session = requests.session()
retry = Retry(connect=10, backoff_factor=2)
adapter = HTTPAdapter(max_retries=retry)
session.mount('http://',adapter)

while True:
    weather_data    = session.get(URL).json()
    # weather_data    = requests.get(URL).json()
    # city        =weather_data['name']
    # print(weather_data)

    temp            = weather_data['main']['temp']
    feels_like      = weather_data['main']['feels_like']
    humidity        = weather_data['main']['humidity']
    weather         = weather_data['weather'][0]['description']

    with open(file,'w') as f:
        # f.write('%s' % (temp) + '°C \n')
        f.write('%s' % round(feels_like) + '°C\n')
        f.write('%s' % (weather))
        # f.write("loop")
        
                

    # print("loop")
    time.sleep(120) # 2 min / call 

    
