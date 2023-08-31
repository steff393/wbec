// Copyright (c) 2023 steff393, MIT license

export function initNavBar() {
	for (const element of document.querySelectorAll('[top-nav-link]')) {
		element.addEventListener('click', function() {window.location.href = element.getAttribute('top-nav-link')});
	}
}

export function assignValuesToHtml(values) {
	let valueContainerElements = document.querySelectorAll('[data-value]');
	for (const element of valueContainerElements) {
		const key = element.getAttribute('data-value');
		if (values[key] !== undefined) {
			element.innerHTML = values[key].toLocaleString('de-DE');
		}
	}
}

export function setClass(element, className, state) {
	if (state) {
		element.classList.add(className)
	} else {
		element.classList.remove(className)
	}
}

export function setSectionVisibility(sectionId, isVisible) {
	setClass(document.getElementById(sectionId), 'not-available', !isVisible);
}
