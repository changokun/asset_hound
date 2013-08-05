<?php
echo "trying to create {$_SERVER['REDIRECT_URL']}\n\n";
$file = new stdClass();
$file->extension = strtolower(substr($_SERVER['REDIRECT_URL'], strrpos($_SERVER['REDIRECT_URL'], '.')));
$file->name = strtolower(substr($_SERVER['REDIRECT_URL'], strrpos($_SERVER['REDIRECT_URL'], '/') + 1));
$file->hashes = array();
$help = isset($_REQUEST['verbose']);

// make sure we're not handling file types that we shouldn't and set the header at the same time
switch($file->extension) {
	case ".js":
		header("Content-Type: text/js", true, 200);
	break;
	case ".css":
		header("Content-Type: text/css", true, 200);
	break;
	default:
		die("unsupported file type.");
	break;
}

// load our settings
if(file_exists($_SERVER['limonada_root'] . '/../limonada-config.inc')) {
    include_once($_SERVER['limonada_root'] . '/../limonada-config.inc');
} else {
    die('config file not found.');
}

//extract the hashes
foreach(explode('/', $_SERVER['REDIRECT_URL']) as $fragment) if(strlen($fragment) == 8) $file->hashes[] = $fragment;

if(count($file->hashes)) {
	//now, find the hashes in the dogPile
	$file->dogPileDir = ASSET_HOUND_ROOT_DIR . "/dogPile";
	$file->contents = "";
	foreach($file->hashes as $hash) {
		$hashFile = $file->dogPileDir . '/' . $hash . $file->extension;
		if(file_exists($hashFile)) {
			$file->contents .= "\n/* ------------------------ $hash ----------------------*/\n";
			$file->contents .= file_get_contents($hashFile);
		} else {
			$file->contents .= "\n/* ------------------------ $hash NOT FOUND----------------------*/\n";
			//TODO set up an email/sms warning system
		}
	}
	// now, save the cache file - first mk any nec dirs.

	$absolute_file_dir = $_SERVER['limonada_root'] . '/' . ASSET_HOUND_CACHE_DIR_NAME . '/' . implode('/', $file->hashes);
	
	echo "\$absolute_file_dir: $absolute_file_dir\n\n";

	mkdir($absolute_file_dir, 0777, true);

	$absolute_file_path = $absolute_file_dir . '/' . $file->name;

	file_put_contents($absolute_file_path, $file->contents);
	
	//now, return the data to the user.
	if( ! $help) {
		header("Expires: " . date('r', date('U') + 60 * 60 * 24 * 365));
		ob_end_clean();
		die("/*caching now*/\n" . $file->contents);
	}
	//done
	
} else {
	die("no hashes found.");
}





var_dump($file);

