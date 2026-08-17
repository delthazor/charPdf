import {
    spellBracketSuffixAsParenForm,
    stripSpellNameForLookup,
} from './dnd.js';

function sharePageDepth() {
    const marker = '/c/';
    const idx = window.location.pathname.indexOf(marker);
    if (idx < 0) {
        return 0;
    }
    const after = window.location.pathname.slice(idx + marker.length);
    const segments = after.split('/').filter(Boolean);
    return segments.length;
}

function relativePrefix(depth) {
    if (depth <= 0) {
        return './';
    }
    return '../'.repeat(depth);
}

export function resolveCfgBase() {
    const depth = sharePageDepth();
    return relativePrefix(depth) + 'cfg/';
}

export function resolveSiteBase() {
    return relativePrefix(sharePageDepth());
}

export function getSlugFromPage() {
    const meta = document.querySelector('meta[name="webrender-char"]');
    if (meta && meta.content) {
        return meta.content.trim().toLowerCase();
    }
    const params = new URLSearchParams(window.location.search);
    const fromQuery = params.get('charname');
    if (fromQuery) {
        return fromQuery.trim().toLowerCase();
    }
    return null;
}

export function campaignIdFromSlug(slug) {
    if (!slug) {
        return null;
    }
    const slash = slug.indexOf('/');
    return slash > 0 ? slug.slice(0, slash) : null;
}

async function fetchJson(relativePath) {
    const response = await fetch(relativePath);
    if (!response.ok) {
        throw new Error('Failed to fetch ' + relativePath + ': ' + response.status);
    }
    return response.json();
}

export async function loadCharacterManifest() {
    const base = resolveCfgBase();
    return fetchJson(base + 'characters.json');
}

export function findManifestEntry(manifest, slug) {
    if (!manifest || !Array.isArray(manifest.campaigns)) {
        return null;
    }
    const normalized = slug.trim().toLowerCase();
    for (const campaign of manifest.campaigns) {
        const characters = campaign.characters || [];
        for (const entry of characters) {
            if (entry.slug === normalized) {
                return entry;
            }
        }
    }
    return null;
}

export async function loadCharacterBundle(slug) {
    const base = resolveCfgBase();
    const manifest = await loadCharacterManifest();
    const entry = findManifestEntry(manifest, slug);
    if (!entry) {
        throw new Error('Character not found: ' + slug);
    }

    const [character, traitsCatalog, spellsCatalog] = await Promise.all([
        fetchJson(base + entry.file),
        fetchJson(base + 'config_traits.json'),
        fetchJson(base + 'config_spells.json'),
    ]);

    return {
        entry,
        character,
        traitsByName: buildTraitsIndex(traitsCatalog),
        spellsByName: buildSpellsIndex(spellsCatalog),
        spellsByLowerName: buildSpellsLowerIndex(spellsCatalog),
    };
}

function buildTraitsIndex(catalog) {
    const map = new Map();
    if (!Array.isArray(catalog)) {
        return map;
    }
    for (const item of catalog) {
        if (item.name && !map.has(item.name)) {
            map.set(item.name, item);
        }
    }
    return map;
}

function buildSpellsIndex(catalog) {
    const map = new Map();
    if (!Array.isArray(catalog)) {
        return map;
    }
    for (const item of catalog) {
        if (item.name && !map.has(item.name)) {
            map.set(item.name, item);
        }
    }
    return map;
}

function buildSpellsLowerIndex(catalog) {
    const map = new Map();
    if (!Array.isArray(catalog)) {
        return map;
    }
    for (const item of catalog) {
        if (item.name) {
            const lower = item.name.toLowerCase();
            if (!map.has(lower)) {
                map.set(lower, item.name);
            }
        }
    }
    return map;
}

export function lookupTrait(traitsByName, name) {
    return traitsByName.get(name) || null;
}

function findSpellByKey(spellsByName, spellsByLowerName, key) {
    if (!key) {
        return null;
    }
    if (spellsByName.has(key)) {
        return spellsByName.get(key);
    }
    const canonical = spellsByLowerName.get(key.toLowerCase());
    if (canonical && spellsByName.has(canonical)) {
        return spellsByName.get(canonical);
    }
    return null;
}

/** Match spell lookup order in Utilities.cpp BuildFullSpellsText. */
export function lookupSpell(spellsByName, spellsByLowerName, displayName) {
    const trimmed = displayName.trim();

    let entry = findSpellByKey(spellsByName, spellsByLowerName, trimmed);
    if (entry) {
        return entry;
    }

    const bracketKey = spellBracketSuffixAsParenForm(trimmed);
    if (bracketKey && bracketKey !== trimmed) {
        entry = findSpellByKey(spellsByName, spellsByLowerName, bracketKey);
        if (entry) {
            return entry;
        }
    }

    const strippedKey = stripSpellNameForLookup(trimmed);
    if (strippedKey) {
        entry = findSpellByKey(spellsByName, spellsByLowerName, strippedKey);
        if (entry) {
            return entry;
        }
    }

    return null;
}
