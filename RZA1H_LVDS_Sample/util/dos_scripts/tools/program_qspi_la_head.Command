//
// Forces J-Link to program, even if flash contents already match data to be programmed
//
r
h
g

// Load the application 
exec SetCompareMode = 0
exec SetVerifyDownload = 0

// Download application into QSPI flash

