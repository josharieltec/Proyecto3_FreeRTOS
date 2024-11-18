<?php
/*************************************************************************************************
 *  Created By: Tauseef Ahmad
 *  Created On: 3 April, 2023
 *  
 *  YouTube Video: https://youtu.be/VEN5kgjEuh8
 *  My Channel: https://www.youtube.com/channel/UCOXYfOHgu-C-UfGyDcu5sYw/
 ***********************************************************************************************/
 
$hostname = "localhost"; 
$username = "root"; 
$password = ""; 
$database = "sensors"; 

$conn = mysqli_connect($hostname, $username, $password, $database);

if (!$conn) { 
	die("Connection failed: " . mysqli_connect_error()); 
} 

echo "Database connection is OK<br>"; 

if(isset($_POST["temperature"]) && isset($_POST["humidity"]) && isset($_POST["co"]) && isset($_POST["smoke"])) {

	$t = $_POST["temperature"];
	$h = $_POST["humidity"];
	$c = $_POST["co"];
	$s = $_POST["smoke"];

	$sql = "INSERT INTO test (temperature, humidity, co, smoke) VALUES (".$t.", ".$h.", ".$c.", ".$s.")"; 

	if (mysqli_query($conn, $sql)) { 
		echo "\nNew record created successfully"; 
	} else { 
		echo "Error: " . $sql . "<br>" . mysqli_error($conn); 
	}
}

?>