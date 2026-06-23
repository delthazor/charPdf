import {
    getSlugFromPage,
    loadCharacterBundle,
    loadCharacterManifest,
    resolveSiteBase,
} from './data.js';
import {
    renderCharacterSheet,
    renderError,
    renderLanding,
    renderLoading,
    updateDocumentTitle,
} from './render.js';

async function bootstrap() {
    const app = document.getElementById('app');
    if (!app) {
        return;
    }

    const siteBase = resolveSiteBase();
    const slug = getSlugFromPage();

    if (!slug) {
        renderLoading(app);
        try {
            const manifest = await loadCharacterManifest();
            renderLanding(app, manifest, siteBase);
        } catch (err) {
            renderError(app, 'Failed to load character list.', siteBase);
            console.error(err);
        }
        return;
    }

    renderLoading(app);
    try {
        const bundle = await loadCharacterBundle(slug);
        updateDocumentTitle(bundle.character);
        renderCharacterSheet(app, bundle);
    } catch (err) {
        const message = err.message && err.message.startsWith('Character not found')
            ? err.message
            : 'Failed to load character data';
        renderError(app, message, siteBase);
        console.error(err);
    }
}

bootstrap();
