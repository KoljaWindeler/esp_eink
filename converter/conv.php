	<?php

	$new_width = 800;
	$new_height = 600;
	$moveToDir ="./";
	$start=time();

	$path = $_GET['path'];
	if(empty($path)){
		exit();
	}

	$mime = getimagesize($path);

	if($mime['mime']=='image/png') {
	    $src_img = imagecreatefrompng($path);
	}
	if($mime['mime']=='image/jpg' || $mime['mime']=='image/jpeg' || $mime['mime']=='image/pjpeg') {
	    $src_img = imagecreatefromjpeg($path);
	}

	$old_x          =   imageSX($src_img);
	$old_y          =   imageSY($src_img);

	if($old_x > $old_y)
	{
	    $thumb_w    =   $new_width;
	    $thumb_h    =   $old_y*($new_width/$old_x);
	}

	if($old_x < $old_y)
	{
	    $thumb_w    =   $old_x*($new_height/$old_y);
	    $thumb_h    =   $new_height;
	}

	if($old_x == $old_y)
	{
	    $thumb_w    =   $new_width;
	    $thumb_h    =   $new_height;
	}

	//echo "new frame shall be ".$thumb_w." x ".$thumb_h."<br>";

	$dst_img        =   ImageCreateTrueColor($new_width,$new_height);//$thumb_w,$thumb_h);
	imagefill($dst_img,0,0,imagecolorallocate($dst_img,255,255,255));
	imagecopyresampled($dst_img,$src_img,($new_width-$thumb_w)/2,($new_height-$thumb_h)/2,0,0,$thumb_w,$thumb_h,$old_x,$old_y);
	imagefilter($dst_img,IMG_FILTER_GRAYSCALE);

	$t1=time()-$start;

	$max_brightness = 16;
	// init array 1.920.000 Byte
	$data = array_fill(0,$new_width*$new_height*$max_brightness/4,0x00);

	$t2=time()-$start;

	for($y=0; $y<$new_height; $y++){ //$thumb_h
		for($x=0; $x<$new_width; $x++){ //$thumb_w
			//echo($x." and ".$y." ");
			$color = imagecolorat($dst_img,$x,$y);
			$r = ($color >> 16) & 0xFF;
			$g = ($color >> 8) & 0xFF;
			$b = ($color) & 0xFF;
			//echo(" color is ".$color." which is ".$r."/".$g."/".$b."<br>");
			$color = (floor($r/16)*16 << 16) | (floor($g/16)*16 << 8) | floor($b/16)*16;

			imagesetpixel($dst_img,$x,$y,$color);

			// prepare file
			for($brightness = 0; $brightness < $max_brightness; $brightness++){ // loop 16 time for this pixel, all color steps
				if(round($b/$max_brightness) > $brightness){ //c/max = 0 to 15.9375 -> 0.499 will stay all white, 15.5 will turn all black
					// first pixel must be shifted to 0b [01]XX XXXX so 6 bits
					$pos = ($x+$y*$new_width+$brightness*($new_width*$new_height));
					$byte_pos = $pos/4;
					$in_byte = $pos%4; // 0,1,2,3 .. 0,1,2,3
					$data[$byte_pos]=$data[$byte_pos]|(0b1<<(2*(3-$in_byte)));  // 0b1 << 2*3 = 0100 0000 // 0b1 = draw black
				}
			}
		}
	}
	$t3=time()-$start;

	$fp = fopen($moveToDir.'data.txt', 'w');
	for($i=0;$i<count($data);$i++){
		fwrite($fp, chr($data[$i]));
	}
	fclose($fp);
	$t4=time()-$start;
	// New save location
	$new_thumb_loc = $moveToDir . "new_" .  basename($path);

	if($mime['mime']=='image/png') {
	    $result = imagepng($dst_img,$new_thumb_loc,8);
	}
	if($mime['mime']=='image/jpg' || $mime['mime']=='image/jpeg' || $mime['mime']=='image/pjpeg') {
	    $result = imagejpeg($dst_img,$new_thumb_loc,80);
	}

	imagedestroy($dst_img);
	imagedestroy($src_img);
	$t5=time()-$start;

	echo "ok <br>";
	echo("total lenght: ".count($data)."<br>");
	echo $t1." till opened file<br>";
	echo $t2." till prepared<br>";
	echo $t3." till converted<br>";
	echo $t4." till stored converted<br>";
	echo $t5." till stored image<br>";

?>
