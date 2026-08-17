import {
    campaignIdFromSlug,
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

function getCampaignFromQuery() {
    const params = new URLSearchParams(window.location.search);
    const campaign = params.get('campaign');
    return campaign ? campaign.trim() : null;
}

async function bootstrap() {
    const app = document.getElementById('app');
    if (!app) {
        return;
    }

    const siteBase = resolveSiteBase();
    const slug = getSlugFromPage();
    const campaign = getCampaignFromQuery() || campaignIdFromSlug(slug);

    if (!slug) {
        renderLoading(app);
        try {
            const manifest = await loadCharacterManifest();
            renderLanding(app, manifest, siteBase, campaign);
        } catch (err) {
            renderError(app, 'Failed to load character list.', siteBase, campaign);
            console.error(err);
        }
        return;
    }

    renderLoading(app);
    try {
        const bundle = await loadCharacterBundle(slug);
        updateDocumentTitle(bundle.character);
        renderCharacterSheet(app, bundle, siteBase);
    } catch (err) {
        const message = err.message && err.message.startsWith('Character not found')
            ? err.message
            : 'Failed to load character data';
        renderError(app, message, siteBase, campaign);
        console.error(err);
    }
}

bootstrap();
