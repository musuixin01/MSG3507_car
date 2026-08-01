$file = "D:\E_Lab_Base\02_Projects\Keil\MSP3507\MSG3507_car\App\line_follow.c"
$content = [System.IO.File]::ReadAllText($file)
$content = $content.Replace("LINE_LOST_HOLD_SPEED_PERCENT);", "g_speedCommand);  /* inherit */")
$content = $content.Replace("LINE_LOST_SEARCH_BASE_PERCENT);", "g_speedCommand * 0.85f);  /* slight decel */")
[System.IO.File]::WriteAllText($file, $content)
Write-Host "Done"
