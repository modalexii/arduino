// portal_page.h: the self-contained set-time HTML/JS page as a raw string literal
// (CLOCK_PLAN.md §2, §5.7). No external assets: the phone has no internet on the clock's AP.
//
// On load it POSTs the phone's clock to /set as JSON:
//   {"utc_ms": Date.now(), "tz": "<IANA name>", "offset_min": getTimezoneOffset()}
// and shows the clock's reply in large text. The button re-sends.
#pragma once

static const char PORTAL_PAGE[] = R"HTML(<!DOCTYPE html>
<html lang="en"><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Clock</title>
<style>
html,body{margin:0;height:100%;background:#000;color:#ddd;
  font-family:-apple-system,system-ui,Roboto,"Segoe UI",sans-serif;text-align:center}
main{display:flex;flex-direction:column;justify-content:center;align-items:center;
  min-height:100%;padding:24px;box-sizing:border-box;gap:20px}
#ring{width:96px;height:96px;border-radius:50%;border:8px solid #ff8a1e;box-sizing:border-box;
  opacity:.35;transition:opacity .4s}
#ring.ok{opacity:1}
#msg{font-size:30px;line-height:1.25;color:#fff;max-width:20em}
#sub{font-size:17px;color:#888;max-width:24em;min-height:1.3em}
button{font-size:22px;padding:16px 40px;border-radius:40px;border:0;background:#ff8a1e;color:#000;
  font-weight:600;-webkit-appearance:none}
button:active{opacity:.7}
</style></head>
<body><main>
<div id="ring"></div>
<div id="msg">Setting the clock&hellip;</div>
<div id="sub"></div>
<button id="btn" type="button">Set time</button>
</main>
<script>
(function(){
  var msg=document.getElementById('msg'),sub=document.getElementById('sub'),
      ring=document.getElementById('ring'),btn=document.getElementById('btn'),busy=false;
  function zone(){try{return Intl.DateTimeFormat().resolvedOptions().timeZone||''}catch(e){return ''}}
  function done(ok,text,detail){busy=false;msg.textContent=text;sub.textContent=detail||'';
    ring.className=ok?'ok':'';}
  function send(){
    if(busy)return;busy=true;
    msg.textContent='Setting the clock…';sub.textContent='';
    var body=JSON.stringify({utc_ms:Date.now(),tz:zone(),offset_min:new Date().getTimezoneOffset()});
    var x=new XMLHttpRequest();
    x.open('POST','/set',true);
    x.setRequestHeader('Content-Type','application/json');
    x.timeout=8000;
    x.onload=function(){
      var r={};try{r=JSON.parse(x.responseText)}catch(e){}
      if(x.status==200&&r.ok)done(true,'✓ Time set to '+r.local,
        (r.tz?r.tz+'. ':'')+'The clock turns its Wi-Fi off in a few seconds; you can close this.');
      else done(false,'Didn’t work',(r.error||('HTTP '+x.status))+'. Tap Set time to retry.');
    };
    x.onerror=x.ontimeout=function(){done(false,'No reply from the clock',
      'Check you’re still on the “Clock” Wi-Fi, then tap Set time.');};
    x.send(body);
  }
  btn.onclick=send;
  if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',send);else send();
})();
</script>
</body></html>
)HTML";
