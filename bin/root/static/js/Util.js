function renderAddthisShareButton(link, descrption) {
	addthis.toolbox("#voteShare",  {}, {url: link, title: descrption});
}

function renderAddthisMainShareButton() {
	addthis.toolbox(".fpMainShare", {}, {url: "www.walvomo.fi"});
}

function piwikTrackUser() {
	try {
		var pkBaseURL = (("https:" == document.location.protocol) ? "https://stbd.no-ip.biz/piwik/" : "http://stbd.no-ip.biz/piwik/");
		var piwikTracker = Piwik.getTracker(pkBaseURL + "piwik.php", 1);
		piwikTracker.trackPageView();
		piwikTracker.enableLinkTracking();
	} catch( err ) {}
}
