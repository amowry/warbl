/*
CLCP v2.1 Clear Links to Current Page
Jonathan Snook
This code is offered unto the public domain
http://www.snook.ca/jonathan/
*/

window.onload = clearCurrentLink;

function clearCurrentLink(){
    var a = document.getElementsByTagName("A");
    for(var i=0;i<a.length;i++)
        if(a[i].href == window.location.href.split("#")[0])
            removeNode(a[i]);
}

function removeNode(n){
    if(n.hasChildNodes())
        for(var i=0;i<n.childNodes.length;i++)
            n.parentNode.insertBefore(n.childNodes[i].cloneNode(true),n);
    n.parentNode.removeChild(n);
}