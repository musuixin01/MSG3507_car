 = "D:\E_Lab_Base\02_Projects\Keil\MSP3507\MSG3507_car\empty.syscfg"
 = Get-Content  -Raw

# Remove ADC12 module import line
 =  -replace "const ADC12  = scripting\.addModule\(""/ti/driverlib/ADC12"", \{\}, false\);\r\n", ""

# Remove TRACK_ADC1 block
 =  -replace "(?s)/\*[\r\n] \* Six analog tracking channels\..*?adcPin5\.\ = ""PB18"";[\r\n][\r\n]", ""

# Remove TRACK_ADC0 block
 =  -replace "(?s)const TRACK_ADC0 = ADC12\.addInstance\(\).*?adcPin7\.\ = ""PA22"";[\r\n][\r\n]", ""

Set-Content  
Write-Host "Done - removed all ADC12 instances from syscfg"
