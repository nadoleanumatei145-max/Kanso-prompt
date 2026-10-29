// Copy-to-clipboard buttons
function copyText(text) {
  if (navigator.clipboard && window.isSecureContext) {
    return navigator.clipboard.writeText(text);
  }
  return new Promise(function (resolve, reject) {
    var ta = document.createElement('textarea');
    ta.value = text;
    ta.setAttribute('readonly', '');
    ta.style.position = 'fixed';
    ta.style.top = '-1000px';
    ta.style.opacity = '0';
    document.body.appendChild(ta);
    ta.select();
    try {
      document.execCommand('copy') ? resolve() : reject(new Error('copy failed'));
    } catch (err) {
      reject(err);
    } finally {
      document.body.removeChild(ta);
    }
  });
}

document.querySelectorAll('[data-copy]').forEach(function (btn) {
  btn.addEventListener('click', function () {
    var target = document.getElementById(btn.getAttribute('data-copy'));
    if (!target) return;

    copyText(target.textContent).then(function () {
      var original = btn.textContent;
      btn.textContent = 'copied';
      btn.classList.add('is-copied');
      setTimeout(function () {
        btn.textContent = original;
        btn.classList.remove('is-copied');
      }, 1600);
    }).catch(function () {
      btn.textContent = 'failed';
      setTimeout(function () { btn.textContent = 'copy'; }, 1600);
    });
  });
});

// GSAP reveal animations
(function () {
  var reduceMotion = window.matchMedia('(prefers-reduced-motion: reduce)').matches;
  var hasGsap = !!(window.gsap && window.ScrollTrigger);

  if (!hasGsap || reduceMotion) {
    document.documentElement.classList.remove('js-loading');
    return;
  }

  gsap.registerPlugin(ScrollTrigger);

  // Explicit initial states (redundant with CSS, but fixes the y offset)
  gsap.set('.hero-anim', { opacity: 0, y: 16 });
  gsap.set('.terminal', { opacity: 0, y: 20, scale: 0.98 });
  gsap.set('.reveal', { opacity: 0, y: 20 });

  // Safe to drop the CSS guard now — GSAP owns opacity from here on
  document.documentElement.classList.remove('js-loading');

  // Hero entry
  if (document.querySelector('.hero-anim')) {
    gsap.to('.hero-anim', {
      opacity: 1, y: 0, duration: 0.7, stagger: 0.09,
      ease: 'power3.out', delay: 0.05
    });
  }
  if (document.querySelector('.terminal')) {
    gsap.to('.terminal', {
      opacity: 1, y: 0, scale: 1, duration: 0.8,
      ease: 'power3.out', delay: 0.2
    });
  }

  // Feature rows — stagger when the container enters view
  if (document.querySelector('.features')) {
    gsap.to('.feature', {
      opacity: 1, y: 0, duration: 0.55, stagger: 0.07, ease: 'power2.out',
      scrollTrigger: { trigger: '.features', start: 'top 82%', once: true }
    });
  }

  // Everything else, individually
  gsap.utils.toArray('.reveal').forEach(function (el) {
    if (el.closest('.features')) return;
    gsap.to(el, {
      opacity: 1, y: 0, duration: 0.6, ease: 'power2.out',
      scrollTrigger: { trigger: el, start: 'top 88%', once: true }
    });
  });
})();
