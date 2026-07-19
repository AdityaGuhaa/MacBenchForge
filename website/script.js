document.addEventListener('DOMContentLoaded', () => {
    // Mobile Menu Toggle
    const navToggle = document.getElementById('nav-toggle');
    const mobileMenu = document.getElementById('mobile-menu');
    
    if (navToggle && mobileMenu) {
        navToggle.addEventListener('click', () => {
            mobileMenu.classList.toggle('active');
            // Toggle hamburger to X (simple visual toggle)
            const spans = navToggle.querySelectorAll('span');
            if (mobileMenu.classList.contains('active')) {
                spans[0].style.transform = 'translateY(7px) rotate(45deg)';
                spans[1].style.opacity = '0';
                spans[2].style.transform = 'translateY(-7px) rotate(-45deg)';
            } else {
                spans[0].style.transform = 'none';
                spans[1].style.opacity = '1';
                spans[2].style.transform = 'none';
            }
        });

        // Close menu on link click
        mobileMenu.querySelectorAll('a').forEach(link => {
            link.addEventListener('click', () => {
                mobileMenu.classList.remove('active');
                const spans = navToggle.querySelectorAll('span');
                spans[0].style.transform = 'none';
                spans[1].style.opacity = '1';
                spans[2].style.transform = 'none';
            });
        });
    }

    // Navbar Scroll Effect
    const navbar = document.getElementById('navbar');
    window.addEventListener('scroll', () => {
        if (window.scrollY > 50) {
            navbar.classList.add('scrolled');
        } else {
            navbar.classList.remove('scrolled');
        }
    });

    // Screenshot Tabs
    const tabs = document.querySelectorAll('.ss-tab');
    const images = document.querySelectorAll('.ss-img');

    tabs.forEach(tab => {
        tab.addEventListener('click', () => {
            // Remove active class from all
            tabs.forEach(t => t.classList.remove('active'));
            images.forEach(img => img.classList.remove('active'));

            // Add active class to clicked tab and target image
            tab.classList.add('active');
            const targetId = tab.getAttribute('data-target');
            document.getElementById(targetId).classList.add('active');
        });
    });

    // Copy Install Command
    const btnCopy = document.getElementById('btn-copy-install');
    if (btnCopy) {
        btnCopy.addEventListener('click', () => {
            const code = `git clone --recursive https://github.com/AdityaGuhaa/MacBenchForge.git
cd MacBenchForge
brew install curl openssl cmake llama.cpp
mkdir build && cd build
cmake ..
cmake --build . -j
./bin/MacBenchForge`;
            
            navigator.clipboard.writeText(code).then(() => {
                const icon = btnCopy.innerHTML;
                btnCopy.innerHTML = `<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="#2ecc71" stroke-width="2"><polyline points="20 6 9 17 4 12"></polyline></svg>`;
                setTimeout(() => {
                    btnCopy.innerHTML = icon;
                }, 2000);
            });
        });
    }

    // Mouse parallax effect for hero screenshot
    const heroVisual = document.querySelector('.hero-visual');
    const heroScreenshot = document.querySelector('.hero-screenshot');
    
    if (heroVisual && heroScreenshot) {
        heroVisual.addEventListener('mousemove', (e) => {
            const rect = heroVisual.getBoundingClientRect();
            const x = e.clientX - rect.left;
            const y = e.clientY - rect.top;
            
            const centerX = rect.width / 2;
            const centerY = rect.height / 2;
            
            const rotateX = ((y - centerY) / centerY) * -5;
            const rotateY = ((x - centerX) / centerX) * 5;
            
            heroScreenshot.style.transform = `rotateX(${rotateX}deg) rotateY(${rotateY}deg) translateY(-10px)`;
        });
        
        heroVisual.addEventListener('mouseleave', () => {
            heroScreenshot.style.transform = 'rotateX(5deg) translateY(0)';
        });
    }
});
