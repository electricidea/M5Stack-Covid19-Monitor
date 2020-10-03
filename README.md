# Covid-19 Data Monitor
Using the M5Stack to visualize timeseries data of the Covid-19 pandemic.

![M5StickC](/images/M5Stack_Covid19_monitor.jpg)

Timeseries data of confirmed infections and deaths during the Covid-19 pandemic strongly illustrate the seriousness of the current situation.
Many websites or smartphone apps are available to view and compare this data. Interfaces to current data sets are also available, which makes it possible to evaluate and display the data by yourself.
I was interested in comparing the data of certain countries directly in one graph. So I wrote a software to display the data of 5 countries as combined graphs on a M5Stack.

The project page on hackster.io can be found here:
[hackster.io page](https://www.hackster.io/hague/covid-19-data-monitor-dfd267)

A video of the M5Stack Covid-19 in action can be found here:
[youtube video](https://youtu.be/79UIikXWLLQ)

Since version 1.09 screenshots of the displayed images can be saved to SD card:

![M5StickC](/images/graph_1.png) ![M5StickC](/images/graph_2.png)

Changelog:  
v1.3  
   = first published version  
v1.4  
   = Bugfix Screen height in graph routine  
   = Changed color order  
v1.05  
   = Store data of multiple countries (30)  
   = Load data after WiFi connection  
   = Included weekly grid lines in graph  
   = The entries are now editable  
   = Bugfix scaling x-axis in Graph  
v1.06  
   = Added shifted graph analysis  
v1.07  
   = Added Europe analysis  
   = add graph for "All Countries"  
   = Auto Display dimming function  
   = changed shifted threshold to 4000 (confirmed) and 500 (deaths)  
v1.08  
   = aded screen capture onto SD card
v1.09  
   = aded smaller demo file to https://electricidea.github.io/  
   = Option to load real data or short test data (36kB)  
   = screen capute is let or right button is pressed for 2 seconds  
