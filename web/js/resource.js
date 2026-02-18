window.addEventListener('DOMContentLoaded', () => {
    if (!client.isAuthenticated()) {
        window.location.href = '/index.html';
        return;
    }

    const resourceId = UI.getUrlParam('id');
    const resourceName = UI.getUrlParam('name');
    if (!resourceId) {
        window.location.href = '/home.html';
        return;
    }

    document.getElementById('resource-name').textContent = resourceName || 'Resource';
    document.title = `${resourceName || 'Resource'} - Flashback`;

    const signoutBtn = document.getElementById('signout-btn');
    if (signoutBtn) {
        signoutBtn.addEventListener('click', async (e) => {
            e.preventDefault();
            await client.signOut();
            window.location.href = '/index.html';
        });
    }

    const addSectionBtn = document.getElementById('add-section-btn');
    if (addSectionBtn) {
        addSectionBtn.addEventListener('click', (e) => {
            e.preventDefault();
            UI.toggleElement('add-section-form', true);
            setTimeout(() => {
                const nameInput = document.getElementById('section-name');
                if (nameInput) {
                    nameInput.focus();
                }
            }, 100);
        });
    }

    const cancelSectionBtn = document.getElementById('cancel-section-btn');
    if (cancelSectionBtn) {
        cancelSectionBtn.addEventListener('click', () => {
            UI.toggleElement('add-section-form', false);
            UI.clearForm('section-form');
        });
    }

    const sectionForm = document.getElementById('section-form');
    if (sectionForm) {
        sectionForm.addEventListener('submit', async (e) => {
            e.preventDefault();

            const name = document.getElementById('section-name').value.trim();
            const link = document.getElementById('section-link').value.trim();

            if (!name) {
                UI.showError('Please enter a section name');
                return;
            }

            UI.hideMessage('error-message');
            UI.setButtonLoading('save-section-btn', true);

            try {
                // Position 0 means add to end
                await client.createSection(resourceId, name, link, 0);

                UI.toggleElement('add-section-form', false);
                UI.clearForm('section-form');
                UI.setButtonLoading('save-section-btn', false);

                loadSections();
            } catch (err) {
                console.error('Add section failed:', err);
                UI.showError(err.message || 'Failed to add section');
                UI.setButtonLoading('save-section-btn', false);
            }
        });
    }

    loadSections();
});

async function loadSections() {
    UI.toggleElement('loading', true);
    UI.toggleElement('sections-list', false);
    UI.toggleElement('empty-state', false);

    try {
        const resourceId = UI.getUrlParam('id');
        const sections = await client.getSections(resourceId);

        UI.toggleElement('loading', false);

        if (sections.length === 0) {
            UI.toggleElement('empty-state', true);
        } else {
            UI.toggleElement('sections-list', true);
            renderSections(sections);
        }
    } catch (err) {
        console.error('Loading sections failed:', err);
        UI.toggleElement('loading', false);
        UI.showError('Failed to load sections: ' + (err.message || 'Unknown error'));
    }
}

function renderSections(sections) {
    const container = document.getElementById('sections-list');
    container.innerHTML = '';

    // Sort sections by position
    const sortedSections = sections.sort((a, b) => a.position - b.position);

    sortedSections.forEach(section => {
        const sectionItem = document.createElement('div');
        sectionItem.className = 'section-item';
        sectionItem.style.cursor = 'pointer';

        let html = `<div class="section-name">${UI.escapeHtml(section.name)}</div>`;

        if (section.link) {
            html += `
                <div class="section-link">
                    <a href="${UI.escapeHtml(section.link)}" target="_blank" rel="noopener noreferrer" onclick="event.stopPropagation()">
                        ${UI.escapeHtml(section.link)}
                    </a>
                </div>
            `;
        }

        sectionItem.innerHTML = html;

        sectionItem.addEventListener('click', () => {
            const resourceId = UI.getUrlParam('id');
            window.location.href = `section-cards.html?resourceId=${resourceId}&sectionPosition=${section.position}&name=${encodeURIComponent(section.name)}`;
        });

        container.appendChild(sectionItem);
    });
}
