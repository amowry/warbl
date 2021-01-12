/* Toggle between adding and removing the "responsive" class to topnav when the user clicks on the icon */
function topNavFunction() {
    var x = document.getElementById("myTopnav");
    if (x.className === "topnav") {
        x.className += " responsive";
		document.getElementById("bgnd").style.top = "0px";
    } else {
        x.className = "topnav";
		document.getElementById("bgnd").style.top = "-52px";
    }
	
	}
	
	
//This is all to keep the backgroind feom resizing on mobile.	


if ("ontouchstart" in document.documentElement) {
    document.body.classList.add('touch-device');
 //   console.log("touch");

} else {
    document.body.classList.add('hover-device');
}


if (jQuery('body').hasClass("touch-device")) {
//Loading height on touch-device
    function calcFullHeight() {
        jQuery('.woodbg').css("height", $(window).height() + 200);
    }

    (function($) {
        calcFullHeight();

        jQuery(window).on('orientationchange', function() {
            // 500ms timeout for getting the correct height after orientation change
            setTimeout(function() {
                calcFullHeight();
            }, 500);

        });
    })(jQuery);

} else {
    jQuery('.woodbg').css("height", "100vh");


}








// JavaScript Document