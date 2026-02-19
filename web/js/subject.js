window.addEventListener('DOMContentLoaded', () => {
    if (!client.isAuthenticated()) {
        window.location.href = '/index.html';
        return;
    }

    const subjectId = UI.getUrlParam('id');
    const subjectName = UI.getUrlParam('name');
    const roadmapId = UI.getUrlParam('roadmapId');

    if (!subjectId) {
        window.location.href = '/home.html';
        return;
    }

    document.getElementById('subject-name').textContent = subjectName || 'Subject';
    document.title = `${subjectName || 'Subject'} - Flashback`;

    // Display breadcrumb
    displayBreadcrumb(roadmapId);

    const signoutBtn = document.getElementById('signout-btn');
    if (signoutBtn) {
        signoutBtn.addEventListener('click', async (e) => {
            e.preventDefault();
            await client.signOut();
            window.location.href = '/index.html';
        });
    }

    // Practice mode functionality
    const startPracticeBtn = document.getElementById('start-practice-btn');
    if (startPracticeBtn) {
        if (!roadmapId) {
            startPracticeBtn.disabled = true;
            startPracticeBtn.title = 'Access this subject from a roadmap to enable practice mode';
            startPracticeBtn.style.opacity = '0.5';
            startPracticeBtn.style.cursor = 'not-allowed';
        } else {
            startPracticeBtn.addEventListener('click', async (e) => {
                e.preventDefault();
                await startPracticeMode();
            });
        }
    }

    // Tab switching
    const tabs = document.querySelectorAll('.tab');
    const tabContents = document.querySelectorAll('.tab-content');

    tabs.forEach(tab => {
        tab.addEventListener('click', () => {
            const targetTab = tab.dataset.tab;

            // Update active tab
            tabs.forEach(t => t.classList.remove('active'));
            tab.classList.add('active');

            // Update active content
            tabContents.forEach(content => {
                content.classList.remove('active');
            });
            document.getElementById(`${targetTab}-content`).classList.add('active');

            // Load data when switching tabs
            if (targetTab === 'topics' && !topicsLoaded) {
                loadTopics();
            } else if (targetTab === 'resources' && !resourcesLoaded) {
                loadResources();
            }
        });
    });

    // Topics functionality
    const addTopicBtn = document.getElementById('add-topic-btn');
    if (addTopicBtn) {
        addTopicBtn.addEventListener('click', (e) => {
            e.preventDefault();
            UI.toggleElement('add-topic-form', true);
            setTimeout(() => {
                const nameInput = document.getElementById('topic-name');
                if (nameInput) {
                    nameInput.focus();
                }
            }, 100);
        });
    }

    const cancelTopicBtn = document.getElementById('cancel-topic-btn');
    if (cancelTopicBtn) {
        cancelTopicBtn.addEventListener('click', () => {
            UI.toggleElement('add-topic-form', false);
            UI.clearForm('topic-form');
        });
    }

    const topicForm = document.getElementById('topic-form');
    if (topicForm) {
        topicForm.addEventListener('submit', async (e) => {
            e.preventDefault();

            const name = document.getElementById('topic-name').value.trim();
            const levelRadio = document.querySelector('input[name="topic-level"]:checked');
            const level = levelRadio ? parseInt(levelRadio.value) : 0;

            if (!name) {
                UI.showError('Please enter a topic name');
                return;
            }

            UI.hideMessage('error-message');
            UI.setButtonLoading('save-topic-btn', true);

            try {
                // Position 0 means add to end
                await client.createTopic(subjectId, name, level, 0);

                UI.toggleElement('add-topic-form', false);
                UI.clearForm('topic-form');
                UI.setButtonLoading('save-topic-btn', false);

                loadTopics();
            } catch (err) {
                console.error('Add topic failed:', err);
                UI.showError(err.message || 'Failed to add topic');
                UI.setButtonLoading('save-topic-btn', false);
            }
        });
    }

    // Pattern options per resource type
    const patternsByType = {
        0: [{value: 0, label: 'Chapter'}, {value: 1, label: 'Page'}, {value: 7, label: 'Section'}], // Book
        1: [{value: 1, label: 'Page'}], // Website
        2: [{value: 2, label: 'Session'}, {value: 3, label: 'Episode'}], // Course
        3: [{value: 3, label: 'Episode'}], // Video
        4: [{value: 4, label: 'Playlist'}, {value: 5, label: 'Post'}], // Channel
        5: [{value: 5, label: 'Post'}], // Mailing List
        6: [{value: 1, label: 'Page'}], // Manual
        7: [{value: 0, label: 'Chapter'}, {value: 1, label: 'Page'}], // Slides
        8: [{value: 6, label: 'Memory'}], // Nerves
    };

    const typeSelect = document.getElementById('resource-type');
    const patternSelect = document.getElementById('resource-pattern');

    typeSelect.addEventListener('change', () => {
        const typeValue = typeSelect.value;
        patternSelect.innerHTML = '';

        if (!typeValue) {
            patternSelect.disabled = true;
            patternSelect.appendChild(new Option('Select type first...', ''));
            return;
        }

        patternSelect.disabled = false;
        const patterns = patternsByType[parseInt(typeValue)] || [];

        if (patterns.length === 1) {
            patternSelect.appendChild(new Option(patterns[0].label, patterns[0].value));
            patternSelect.value = patterns[0].value;
        } else {
            patternSelect.appendChild(new Option('Select pattern...', ''));
            patterns.forEach(p => {
                patternSelect.appendChild(new Option(p.label, p.value));
            });
        }
    });

    // Resources functionality
    const addResourceBtn = document.getElementById('add-resource-btn');
    if (addResourceBtn) {
        addResourceBtn.addEventListener('click', (e) => {
            e.preventDefault();
            UI.toggleElement('add-resource-form', true);
            setTimeout(() => {
                const nameInput = document.getElementById('resource-name');
                if (nameInput) {
                    nameInput.focus();
                }
            }, 100);
        });
    }

    function resetPatternSelect() {
        patternSelect.disabled = true;
        patternSelect.innerHTML = '<option value="">Select type first...</option>';
    }

    const cancelResourceBtn = document.getElementById('cancel-resource-btn');
    if (cancelResourceBtn) {
        cancelResourceBtn.addEventListener('click', () => {
            UI.toggleElement('add-resource-form', false);
            UI.clearForm('resource-form');
            resetPatternSelect();
        });
    }

    const resourceForm = document.getElementById('resource-form');
    if (resourceForm) {
        resourceForm.addEventListener('submit', async (e) => {
            e.preventDefault();

            const name = document.getElementById('resource-name').value.trim();
            const type = document.getElementById('resource-type').value;
            const pattern = document.getElementById('resource-pattern').value;
            const url = document.getElementById('resource-url').value.trim();
            const productionDate = document.getElementById('resource-production').value;
            const expirationDate = document.getElementById('resource-expiration').value;

            if (!name || !type || !pattern || !productionDate || !expirationDate) {
                UI.showError('Please fill required fields');
                return;
            }

            // Convert dates to epoch seconds
            const production = Math.floor(new Date(productionDate).getTime() / 1000);
            const expiration = Math.floor(new Date(expirationDate).getTime() / 1000);

            UI.hideMessage('error-message');
            UI.setButtonLoading('save-resource-btn', true);

            try {
                const resource = await client.createResource(name, parseInt(type), parseInt(pattern), url, production, expiration);
                await client.addResourceToSubject(subjectId, resource.id);

                UI.toggleElement('add-resource-form', false);
                UI.clearForm('resource-form');
                resetPatternSelect();
                UI.setButtonLoading('save-resource-btn', false);

                loadResources();
            } catch (err) {
                console.error('Add resource failed:', err);
                UI.showError(err.message || 'Failed to add resource');
                UI.setButtonLoading('save-resource-btn', false);
            }
        });
    }

    // Load topics initially (since topics tab is active by default)
    let topicsLoaded = false;
    let resourcesLoaded = false;
    loadTopics();
});

async function loadTopics() {
    UI.toggleElement('topics-loading', true);
    UI.toggleElement('topics-list', false);
    UI.toggleElement('topics-empty-state', false);

    try {
        const subjectId = UI.getUrlParam('id');
        const milestoneLevel = parseInt(UI.getUrlParam('level')) || 0;

        // Fetch topics for all levels from 0 up to and including the milestone level
        const allTopics = [];
        for (let level = 0; level <= milestoneLevel; level++) {
            const topics = await client.getTopics(subjectId, level);
            allTopics.push(...topics);
        }

        UI.toggleElement('topics-loading', false);
        topicsLoaded = true;

        if (allTopics.length === 0) {
            UI.toggleElement('topics-empty-state', true);
        } else {
            UI.toggleElement('topics-list', true);
            renderTopics(allTopics, milestoneLevel);
        }
    } catch (err) {
        console.error('Loading topics failed:', err);
        UI.toggleElement('topics-loading', false);
        UI.showError('Failed to load topics: ' + (err.message || 'Unknown error'));
    }
}

function renderTopics(topics, maxLevel) {
    const container = document.getElementById('topics-list');
    container.innerHTML = '';

    const levelInfo = [
        { name: 'Surface', description: 'Complete understanding' },
        { name: 'Depth', description: 'In-depth details' },
        { name: 'Origin', description: 'Creator level' }
    ];

    // Group topics by level
    const topicsByLevel = { 0: [], 1: [], 2: [] };
    topics.forEach(topic => {
        if (topicsByLevel[topic.level] !== undefined) {
            topicsByLevel[topic.level].push(topic);
        }
    });

    // Render levels from 0 (Surface) up to maxLevel, displaying from bottom to top visually
    // We reverse the order so higher levels appear at top
    const levelsToShow = [];
    for (let i = 0; i <= maxLevel; i++) {
        levelsToShow.push(i);
    }
    levelsToShow.reverse(); // Show Origin -> Depth -> Surface

    levelsToShow.forEach(level => {
        if (topicsByLevel[level] && topicsByLevel[level].length > 0) {
            const levelSection = document.createElement('div');
            levelSection.style.marginBottom = '2rem';

            const levelHeader = document.createElement('div');
            levelHeader.style.cssText = 'display: flex; align-items: center; gap: 1rem; margin-bottom: 1rem; padding-bottom: 0.5rem; border-bottom: 2px solid var(--border-color);';
            levelHeader.innerHTML = `
                <span class="topic-level">${UI.escapeHtml(levelInfo[level].name)}</span>
                <span style="color: var(--text-muted); font-size: 0.9rem;">${UI.escapeHtml(levelInfo[level].description)}</span>
            `;
            levelSection.appendChild(levelHeader);

            // Sort topics by position within each level
            const sortedTopics = topicsByLevel[level].sort((a, b) => a.position - b.position);

            sortedTopics.forEach(topic => {
                const topicItem = document.createElement('div');
                topicItem.className = 'topic-item';
                topicItem.style.cursor = 'pointer';
                topicItem.innerHTML = `
                    <div class="topic-name">${UI.escapeHtml(topic.name)}</div>
                `;

                topicItem.addEventListener('click', () => {
                    const subjectId = UI.getUrlParam('id');
                    const subjectName = UI.getUrlParam('name');
                    const roadmapId = UI.getUrlParam('roadmapId');
                    const roadmapName = UI.getUrlParam('roadmapName');
                    window.location.href = `topic-cards.html?subjectId=${subjectId}&topicPosition=${topic.position}&topicLevel=${topic.level}&name=${encodeURIComponent(topic.name)}&subjectName=${encodeURIComponent(subjectName || '')}&roadmapId=${roadmapId || ''}&roadmapName=${encodeURIComponent(roadmapName || '')}`;
                });

                levelSection.appendChild(topicItem);
            });

            container.appendChild(levelSection);
        }
    });
}

async function loadResources() {
    UI.toggleElement('resources-loading', true);
    UI.toggleElement('resources-list', false);
    UI.toggleElement('resources-empty-state', false);

    try {
        const subjectId = parseInt(UI.getUrlParam('id'));
        const resources = await client.getResources(subjectId);

        UI.toggleElement('resources-loading', false);
        resourcesLoaded = true;

        if (resources.length === 0) {
            UI.toggleElement('resources-empty-state', true);
        } else {
            UI.toggleElement('resources-list', true);
            renderResources(resources);
        }
    } catch (err) {
        console.error('Loading resources failed:', err);
        UI.toggleElement('resources-loading', false);
        UI.showError('Failed to load resources: ' + (err.message || 'Unknown error'));
    }
}

function renderResources(resources) {
    const container = document.getElementById('resources-list');
    container.innerHTML = '';

    const typeNames = ['Book', 'Website', 'Course', 'Video', 'Channel', 'Mailing List', 'Manual', 'Slides', 'Your Knowledge'];
    const patternNames = ['Chapters', 'Pages', 'Sessions', 'Episodes', 'Playlist', 'Posts', 'Memories'];

    resources.forEach(resource => {
        const resourceItem = document.createElement('div');
        resourceItem.className = 'resource-item';
        resourceItem.style.cursor = 'pointer';

        // Convert epoch seconds to readable dates
        const productionDate = resource.production ? new Date(resource.production * 1000).toLocaleDateString() : 'N/A';
        const expirationDate = resource.expiration ? new Date(resource.expiration * 1000).toLocaleDateString() : 'N/A';

        const typeName = typeNames[resource.type] || 'Unknown';
        const patternName = patternNames[resource.pattern] || 'Unknown';

        resourceItem.innerHTML = `
            <div class="resource-header">
                <div class="resource-name">${UI.escapeHtml(resource.name)}</div>
                <span class="resource-type">${UI.escapeHtml(typeName)} ${UI.escapeHtml(patternName)}</span>
            </div>
            <div class="resource-url">
                <a href="${UI.escapeHtml(resource.link)}" target="_blank" rel="noopener noreferrer" onclick="event.stopPropagation()">
                    ${UI.escapeHtml(resource.link)}
                </a>
            </div>
            <div style="margin-top: 0.75rem; color: var(--text-muted); font-size: 0.9rem;">
                <div><strong>Produced:</strong> ${UI.escapeHtml(productionDate)}</div>
                <div><strong>Relevant Until:</strong> ${UI.escapeHtml(expirationDate)}</div>
            </div>
        `;

        // Make the whole resource item clickable to go to resource page
        resourceItem.addEventListener('click', () => {
            const subjectId = UI.getUrlParam('id');
            const subjectName = UI.getUrlParam('name');
            const roadmapId = UI.getUrlParam('roadmapId');
            const roadmapName = UI.getUrlParam('roadmapName');
            window.location.href = `resource.html?id=${resource.id}&name=${encodeURIComponent(resource.name)}&subjectId=${subjectId || ''}&subjectName=${encodeURIComponent(subjectName || '')}&roadmapId=${roadmapId || ''}&roadmapName=${encodeURIComponent(roadmapName || '')}`;
        });

        container.appendChild(resourceItem);
    });
}

async function startPracticeMode() {
    const subjectId = UI.getUrlParam('id');
    const roadmapId = UI.getUrlParam('roadmapId');

    if (!roadmapId || !subjectId) {
        UI.showError('Missing roadmap or subject information');
        return;
    }

    UI.setButtonLoading('start-practice-btn', true);

    try {
        const topics = await client.getPracticeTopics(parseInt(roadmapId), parseInt(subjectId));

        if (topics.length === 0) {
            UI.showError('No topics available for practice');
            UI.setButtonLoading('start-practice-btn', false);
            return;
        }

        // Store practice state in sessionStorage
        sessionStorage.setItem('practiceState', JSON.stringify({
            roadmapId: parseInt(roadmapId),
            subjectId: parseInt(subjectId),
            topics: topics,
            currentTopicIndex: 0,
            practiceMode: 'aggressive'
        }));

        // Find first topic with cards
        let firstTopicWithCards = null;
        let cards = [];

        for (let i = 0; i < topics.length; i++) {
            const topic = topics[i];
            const topicCards = await client.getPracticeCards(
                parseInt(roadmapId),
                parseInt(subjectId),
                topic.level,
                topic.position
            );

            if (topicCards.length > 0) {
                firstTopicWithCards = topic;
                cards = topicCards;
                break;
            }
        }

        if (!firstTopicWithCards || cards.length === 0) {
            UI.showError('No cards available for practice');
            UI.setButtonLoading('start-practice-btn', false);
            return;
        }

        // Update practice state with cards
        const practiceState = JSON.parse(sessionStorage.getItem('practiceState'));
        practiceState.currentCards = cards;
        sessionStorage.setItem('practiceState', JSON.stringify(practiceState));

        // Navigate to first card with context
        const subjectNameParam = UI.getUrlParam('name') || '';
        const roadmapName = UI.getUrlParam('roadmapName') || '';
        window.location.href = `card.html?cardId=${cards[0].id}&headline=${encodeURIComponent(cards[0].headline)}&state=${cards[0].state}&practiceMode=aggressive&cardIndex=0&totalCards=${cards.length}&subjectName=${encodeURIComponent(subjectNameParam)}&topicName=${encodeURIComponent(firstTopicWithCards.name)}&roadmapName=${encodeURIComponent(roadmapName)}`;
    } catch (err) {
        console.error('Start practice failed:', err);
        UI.showError('Failed to start practice: ' + (err.message || 'Unknown error'));
        UI.setButtonLoading('start-practice-btn', false);
    }
}

async function displayBreadcrumb(roadmapId) {
    const breadcrumb = document.getElementById('breadcrumb');
    if (!breadcrumb || !roadmapId) return;

    try {
        const roadmapName = UI.getUrlParam('roadmapName');
        if (roadmapName) {
            breadcrumb.innerHTML = `<a href="roadmap.html?id=${roadmapId}&name=${encodeURIComponent(roadmapName)}" style="color: var(--text-primary); text-decoration: none;">${UI.escapeHtml(roadmapName)}</a>`;
        }
    } catch (err) {
        console.error('Failed to display breadcrumb:', err);
    }
}
