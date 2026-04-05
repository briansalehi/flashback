// Mobile navigation toggle
document.addEventListener('DOMContentLoaded', () => {
    const menuToggle = document.querySelector('.menu-toggle');
    const nav = document.querySelector('.nav');
    const menuOverlay = document.querySelector('.menu-overlay');

    if (menuToggle && nav && menuOverlay) {
        // Add Settings tab with Shortcuts Toggle
        const accountLink = nav.querySelector('a[href="account.html"]');
        if (accountLink) {
            const settingsContainer = document.createElement('div');
            settingsContainer.className = 'nav-settings-container';
            settingsContainer.innerHTML = `
                <a href="#" class="nav-settings-toggle">Settings</a>
                <div class="nav-settings-dropdown">
                    <div class="nav-settings-item">
                        <span>Shortcuts</span>
                        <label class="switch">
                            <input type="checkbox" id="shortcuts-toggle">
                            <span class="slider"></span>
                        </label>
                    </div>
                </div>
            `;
            accountLink.parentNode.insertBefore(settingsContainer, accountLink);

            const shortcutsToggle = document.getElementById('shortcuts-toggle');
            const isEnabled = localStorage.getItem('flashback_shortcuts_enabled') === 'true';
            shortcutsToggle.checked = isEnabled;

            shortcutsToggle.addEventListener('change', (e) => {
                localStorage.setItem('flashback_shortcuts_enabled', e.target.checked);
                // Dispatch event so other scripts can react
                window.dispatchEvent(new CustomEvent('shortcutsChanged', { detail: { enabled: e.target.checked } }));
            });

            // Toggle dropdown on click
            const settingsToggle = settingsContainer.querySelector('.nav-settings-toggle');
            settingsToggle.addEventListener('click', (e) => {
                e.preventDefault();
                settingsContainer.classList.toggle('active');
            });

            // Close dropdown when clicking outside
            document.addEventListener('click', (e) => {
                if (!settingsContainer.contains(e.target)) {
                    settingsContainer.classList.remove('active');
                }
            });
        }

        // Toggle menu on button click
        menuToggle.addEventListener('click', () => {
            menuToggle.classList.toggle('active');
            nav.classList.toggle('active');
            menuOverlay.classList.toggle('active');

            // Prevent body scroll when menu is open
            if (nav.classList.contains('active')) {
                document.body.style.overflow = 'hidden';
            } else {
                document.body.style.overflow = '';
            }
        });

        // Close menu when clicking overlay
        menuOverlay.addEventListener('click', () => {
            menuToggle.classList.remove('active');
            nav.classList.remove('active');
            menuOverlay.classList.remove('active');
            document.body.style.overflow = '';
        });

        // Close menu when clicking a nav link
        const navLinks = nav.querySelectorAll('a');
        navLinks.forEach(link => {
            link.addEventListener('click', () => {
                menuToggle.classList.remove('active');
                nav.classList.remove('active');
                menuOverlay.classList.remove('active');
                document.body.style.overflow = '';
            });
        });
    }
});
