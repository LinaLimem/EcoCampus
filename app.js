/*
 * app.js - Module Web EcoCampus (DSI2 - Digital Minds)
 * Chargement de data.json, filtres, statistiques, graphiques et tableau triable.
 */
'use strict';

/* ------------------------------------------------------------------ */
/* Constantes et utilitaires                                          */
/* ------------------------------------------------------------------ */

const RES = {
    electricite: { label: 'Électricité', color: '#d99a2b' },
    eau:         { label: 'Eau',         color: '#3a7ca5' },
    chauffage:   { label: 'Chauffage',   color: '#b5651d' }
};
const ORDER = ['electricite', 'eau', 'chauffage'];
// Une mesure est "proche du seuil" à partir de 90 % du seuil, sans le dépasser
// (même règle que POURCENT_PROCHE dans le module C).
const PROCHE_RATIO = 90;
const NIVEAUX = ['normal', 'proche_seuil', 'alerte'];

// Clé utilisée pour mémoriser le thème (clair/sombre) dans le navigateur.
const THEME_KEY = 'ecocampus-theme';
const $ = (s, root = document) => root.querySelector(s);
const $$ = (s, root = document) => [...root.querySelectorAll(s)];
const fmtNum = new Intl.NumberFormat('fr-FR', { maximumFractionDigits: 1 }).format;
const fmtInt = new Intl.NumberFormat('fr-FR', { maximumFractionDigits: 0 }).format;
const collator = new Intl.Collator('fr', { sensitivity: 'base' });
const reduced = () => matchMedia('(prefers-reduced-motion: reduce)').matches;

// Normalise un texte pour la recherche : enlève les accents et met en minuscules ("Bâtiment" -> "batiment").
const norm = (v) => String(v).normalize('NFD').replace(/[\u0300-\u036f]/g, '').toLowerCase();
const esc = (v) => String(v).replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;').replace(/'/g, '&#39;');
const empty = (title, msg) => `<div class="empty-state"><strong>${esc(title)}</strong><span>${esc(msg)}</span></div>`;


/**
 * unitFor(m) : détermine l'unité d'une mesure.
 * Prend l'unité du JSON si elle existe, sinon m³ pour l'eau et kWh pour le reste.
 * Corrige aussi "m3" en "m³" pour un bel affichage.
 */
function unitFor(m) {
    const unit = m.unite || (m.type_ressource === 'eau' ? 'm³' : 'kWh');
    return unit === 'm3' ? 'm³' : unit;
}

/**
 * dateShort(value) : transforme "2026-03-15 14:30" en format court "15/03 14:30"
 * (sans l'année). Si le format est invalide, retourne le texte d'origine.
 */
function dateShort(value) {
    const [date, time] = String(value).split(' ');
    const [y, m, d] = (date || '').split('-');
    return y ? `${d}/${m} ${time || ''}`.trim() : String(value);
}

/**
 * dateLong(value) : transforme "2026-03-15 14:30" en format long "15/03/2026 à 14:30".
 * Utilisée dans le pied de page et dans le chatbot (période couverte).
 */
function dateLong(value) {
    const [date, time] = String(value).split(' ');
    const [y, m, d] = (date || '').split('-');
    return y ? `${d}/${m}/${y}${time ? ' à ' + time : ''}` : String(value);
}

/**
 * debounce(fn, wait) : retarde l'exécution d'une fonction.
 * Si elle est appelée plusieurs fois rapidement (ex : frappe dans la recherche),
 * seule la dernière est exécutée après "wait" ms. Évite de recalculer à chaque touche.
 */
function debounce(fn, wait) {
    let timer;
    return (...args) => {
        clearTimeout(timer);
        timer = setTimeout(() => fn(...args), wait);
    };
}

/* ------------------------------------------------------------------ */
/* État et éléments du DOM                                            */
/* ------------------------------------------------------------------ */

// Objet qui contiendra les références vers les éléments HTML (rempli par cacheElements).
const state = { mesures: [], filtered: [], meta: {}, source: '', sort: { key: 'date_heure', dir: -1 } };
const el = {};


/**
 * cacheElements() : récupère une seule fois tous les éléments HTML utiles
 * (boutons, champs de filtre, graphiques, tableau...) et les range dans l'objet "el".
 */
function cacheElements() {
    Object.assign(el, {
        status: $('#data-status'),
        load: $('#load-data'),
        file: $('#file-input'),
        theme: $('#theme-toggle'),
        themeLabel: $('#theme-label'),
        alertCard: $('#alert-card'),
        nearCard: $('#near-card'),
        total: $('#stat-total'),
        moyenne: $('#stat-average'),
        max: $('#stat-max'),
        min: $('#stat-min'),
        alerts: $('#stat-alerts'),
        alertsLabel: $('#stat-alerts-label'),
        near: $('#stat-near'),
        nearLabel: $('#stat-near-label'),
        filters: $('#filters'),
        filtersToggle: $('#filters-toggle'),
        search: $('#search'),
        building: $('#building'),
        resource: $('#resource'),
        dateStart: $('#date-start'),
        dateEnd: $('#date-end'),
        reset: $('#reset-filters'),
        bars: $('#bar-chart'),
        donut: $('#donut-chart'),
        donutTotal: $('#donut-center-value'),
        donutLabel: $('#donut-center-label'),
        legend: $('#pie-legend'),
        body: $('#measures-body'),
        count: $('#table-count'),
        footer: $('#footer-update'),
        toasts: $('#toast-container')
    });
}

/* ------------------------------------------------------------------ */
/* Données : validation, chargement et filtrage                       */
/* ------------------------------------------------------------------ */


/**
 * validate(raw) : vérifie et nettoie les données du JSON.
 * Contrôle pour chaque mesure : nom du bâtiment, type de ressource connu, date valide,
 * valeur >= 0 et seuil > 0. Les mesures invalides sont ignorées (leur numéro de ligne est gardé).
 * Elle calcule aussi l'alerte si elle est absente (valeur > seuil).
 * Retourne { mesures (valides), ignored (numéros des lignes rejetées) }.
 */
function validate(raw) {
    if (!raw || !Array.isArray(raw.mesures))
        throw new Error('Le fichier ne contient pas de tableau "mesures".');

    const mesures = [];
    const ignored = [];

    raw.mesures.forEach((m, index) => {
        const valeur = Number(m && m.valeur_consommation);
        const seuil = Number(m && m.seuil_reference);
        const date = String((m && m.date_heure) || '').replace('T', ' ');
        const ok = m && typeof m.nom_batiment === 'string' && m.nom_batiment.trim() &&
            RES[m.type_ressource] && /^\d{4}-\d{2}-\d{2} \d{2}:\d{2}/.test(date) &&
            Number.isFinite(valeur) && valeur >= 0 && Number.isFinite(seuil) && seuil > 0;

        if (!ok) {
            ignored.push(index + 1);
            return;
        }

        const alerte = typeof m.alerte === 'boolean' ? m.alerte : valeur > seuil;
        const niveau = NIVEAUX.includes(m.niveau)
            ? m.niveau
            : alerte ? 'alerte' : valeur * 100 >= seuil * PROCHE_RATIO ? 'proche_seuil' : 'normal';

        mesures.push({
            ...m,
            nom_batiment: m.nom_batiment.trim(),
            date_heure: date,
            valeur_consommation: valeur,
            seuil_reference: seuil,
            unite: unitFor(m),
            alerte,
            niveau
        });
    });

    if (!mesures.length)
        throw new Error('Aucune mesure valide dans le fichier.');
    return { mesures, ignored };
}

/**
 * apply(raw, source) : applique un jeu de données dans l'application.
 * Valide les données, remplit l'état global, réinitialise filtres et tri,
 * rafraîchit l'affichage, puis affiche des notifications (mesures ignorées, nombre d'anomalies).
 */
function apply(raw, source) {
    const { mesures, ignored } = validate(raw);

    state.mesures = mesures;
    state.meta = (raw.meta && typeof raw.meta === 'object') ? raw.meta : {};
    state.source = source || state.meta.source || 'data.json';
    state.sort = { key: 'date_heure', dir: -1 };

    fillBuildings();
    clearFilters();
    setDateBounds();
    refresh();

    setStatus(`${fmtInt(mesures.length)} mesures chargées`, 'ok');
    updateFooter();

    if (ignored.length)
        toast(`${ignored.length} mesure(s) ignorée(s) (ligne(s) ${ignored.join(', ')}).`, 'warning', 6000);

    const alertes = mesures.filter((m) => m.niveau === 'alerte').length;
    const proches = mesures.filter((m) => m.niveau === 'proche_seuil').length;
    const details = [];
    if (alertes) details.push(`${fmtInt(alertes)} anomalie(s) : les lignes rouges signalent les dépassements de seuil.`);
    if (proches) details.push(`${fmtInt(proches)} mesure(s) proche(s) du seuil (affichées en orange).`);
    toast(
        details.length ? details.join(' ') : 'Aucune anomalie ni mesure proche du seuil détectée dans ce jeu de données.',
        alertes ? 'warning' : proches ? 'info' : 'success',
        6000
    );
}

/**
 * loadData() : charge automatiquement data.json avec fetch (fonction asynchrone).
 * En cas de succès, appelle apply(). En cas d'échec (ex : page ouverte sans serveur local),
 * affiche un message d'erreur et l'état vide.
 */
async function loadData() {
    setStatus('Chargement de data.json…', 'loading');
    try {
        const response = await fetch('data.json', { cache: 'no-store' });
        if (!response.ok) throw new Error('HTTP ' + response.status);
        apply(await response.json(), 'data.json');
    } catch (error) {
        setStatus('data.json indisponible', 'error');
        emptyUI('Données non chargées',
            'Lancez un serveur local (Live Server ou python -m http.server), chargez data.json ou glissez-déposez le fichier.');
        toast('Chargement automatique impossible. Utilisez « Charger data.json ».', 'danger', 6000);
    }
}


/**
 * readFile(file) : lit un fichier .json choisi par l'utilisateur (bouton ou glisser-déposer).
 * Vérifie l'extension, lit le contenu avec FileReader, le convertit avec JSON.parse
 * puis appelle apply(). Gère les erreurs de lecture et de format.
 */
function readFile(file) {
    if (!file) return;
    if (!/\.json$/i.test(file.name)) {
        toast('Veuillez sélectionner un fichier .json.', 'warning');
        return;
    }

    setStatus(`Lecture de ${file.name}…`, 'loading');
    const reader = new FileReader();

    reader.onload = () => {
        try {
            apply(JSON.parse(String(reader.result).replace(/^\uFEFF/, '')), file.name);
        } catch (error) {
            setStatus('Fichier invalide', 'error');
            toast('Fichier invalide : ' + error.message, 'danger', 6000);
        } finally {
            el.file.value = '';
        }
    };
    reader.onerror = () => {
        setStatus('Lecture impossible', 'error');
        toast('La lecture du fichier a échoué.', 'danger');
        el.file.value = '';
    };
    reader.readAsText(file, 'UTF-8');
}

/**
 * filterMesures() : retourne les mesures qui respectent tous les filtres actifs :
 * recherche par nom (sans accents), bâtiment, ressource et période (date de début / de fin).
 */
function filterMesures() {
    const q = norm(el.search.value.trim());
    const building = el.building.value;
    const resource = el.resource.value;
    const start = el.dateStart.value;
    const end = el.dateEnd.value;

    return state.mesures.filter((m) => {
        const day = m.date_heure.slice(0, 10);
        return (!q || norm(m.nom_batiment).includes(q)) &&
            (!building || m.nom_batiment === building) &&
            (!resource || m.type_ressource === resource) &&
            (!start || day >= start) && (!end || day <= end);
    });
}

/* ------------------------------------------------------------------ */
/* Statistiques et indicateurs                                        */
/* ------------------------------------------------------------------ */


/**
 * computeStats(mesures) : calcule les statistiques d'une liste de mesures :
 * total consommé, nombre d'alertes, valeur max, valeur min, moyenne par bâtiment,
 * et le détail par bâtiment (consommation totale + seuil cumulé) pour l'histogramme.
 */
function computeStats(mesures) {
    const buildings = new Map();
    let total = 0, alerts = 0, proches = 0, max = 0, min = Infinity;

    mesures.forEach((m) => {
        total += m.valeur_consommation;
        alerts += m.niveau === 'alerte' ? 1 : 0;
        proches += m.niveau === 'proche_seuil' ? 1 : 0;
        max = Math.max(max, m.valeur_consommation);
        min = Math.min(min, m.valeur_consommation);

        const b = buildings.get(m.nom_batiment) || { nom: m.nom_batiment, total: 0, seuil: 0, alertes: 0, proches: 0 };
        b.total += m.valeur_consommation;
        b.seuil += m.seuil_reference;
        b.alertes += m.niveau === 'alerte' ? 1 : 0;
        b.proches += m.niveau === 'proche_seuil' ? 1 : 0;
        buildings.set(m.nom_batiment, b);
    });

    const list = [...buildings.values()];
    return {
        total,
        alerts,
        proches,
        max,
        min: mesures.length ? min : 0,
        moyenne: list.length ? total / list.length : 0,
        buildings: list
    };
}


/**
 * animate(node, target, format) : anime un compteur de son ancienne valeur vers la nouvelle
 * en 600 ms (effet de ralentissement progressif) avec requestAnimationFrame.
 * Si les animations sont réduites ou si la valeur ne change pas, affiche directement la valeur.
 */
function animate(node, target, format) {
    if (node._raf) cancelAnimationFrame(node._raf);
    const from = Number(node.dataset.value || 0);
    const to = Number(target) || 0;

    if (reduced() || from === to) {
        node.textContent = format(to);
        node.dataset.value = to;
        return;
    }

    const start = performance.now();
    const step = (now) => {
        const p = Math.min((now - start) / 600, 1);
        const eased = 1 - Math.pow(1 - p, 3);
        node.textContent = format(from + (to - from) * eased);
        if (p < 1) {
            node._raf = requestAnimationFrame(step);
        } else {
            node.textContent = format(to);
            node.dataset.value = to;
            node._raf = null;
        }
    };
    node._raf = requestAnimationFrame(step);
}

/**
 * renderIndicators() : met à jour les 5 cartes de statistiques (total, moyenne, max, min, alertes)
 * à partir des mesures filtrées. Met la carte des alertes en rouge s'il y en a et
 * accorde le mot "alerte/alertes" au singulier ou au pluriel.
 */
function renderIndicators() {
    const stats = computeStats(state.filtered);
    animate(el.total, stats.total, fmtNum);
    animate(el.moyenne, stats.moyenne, fmtNum);
    animate(el.max, stats.max, fmtNum);
    animate(el.min, stats.min, fmtNum);
    animate(el.alerts, stats.alerts, fmtInt);
    animate(el.near, stats.proches, fmtInt);
    el.alertCard.classList.toggle('has-alerts', stats.alerts > 0);
    el.alertsLabel.textContent = stats.alerts > 1 ? 'alertes' : 'alerte';
    el.nearCard.classList.toggle('has-near', stats.proches > 0);
    el.nearLabel.textContent = stats.proches > 1 ? 'mesures' : 'mesure';
}

/* ------------------------------------------------------------------ */
/* Graphiques                                                         */
/* ------------------------------------------------------------------ */

/**
 * renderBars() : dessine l'histogramme (en HTML/CSS) : une barre par bâtiment,
 * représentant sa consommation en % de son seuil de référence.
 * Les barres dépassant 100 % sont en rouge et une ligne "Seuil" marque la limite.
 * Affiche un message vide s'il n'y a aucune mesure.
 */
function renderBars() {
    const buildings = computeStats(state.filtered).buildings.sort((a, b) => b.total - a.total);

    if (!buildings.length) {
        el.bars.innerHTML = empty('Aucune mesure à afficher', 'Modifiez les filtres pour visualiser l’histogramme.');
        el.bars.removeAttribute('aria-label');
        return;
    }

    const ratios = buildings.map((b) => (b.seuil ? (b.total / b.seuil) * 100 : 0));
    const scale = Math.max(120, Math.ceil(Math.max(...ratios) / 20) * 20);

    const bars = buildings.map((b) => {
        const ratio = b.seuil ? (b.total / b.seuil) * 100 : 0;
        return { ...b, ratio, level: ratio > 100 ? 'is-danger' : ratio >= PROCHE_RATIO ? 'is-near' : '' };
    });

    const tracks = bars.map((b, i) => {
        const height = Math.max(2, (b.ratio / scale) * 100);
        const level = b.level === 'is-danger' ? ' (au-dessus du seuil)'
            : b.level === 'is-near' ? ' (proche du seuil)' : '';
        const title = `${b.nom} — ${fmtNum(b.total)} consommés pour un seuil de ${fmtNum(b.seuil)} (${Math.round(b.ratio)} %)${level}`;
        return `<div class="bar-track" title="${esc(title)}">
            <div class="bar ${b.level}" style="--h:${height.toFixed(2)}%;--i:${i}">
                <span class="bar-value">${fmtNum(b.total)}</span>
            </div>
        </div>`;
    }).join('');

    const labels = bars.map((b) => {
        const near = b.proches ? ` · ${fmtInt(b.proches)} proche(s)` : '';
        return `<span class="bar-label"><strong>${esc(b.nom)}</strong><em>${Math.round(b.ratio)} % du seuil${near}</em></span>`;
    }).join('');

    el.bars.innerHTML = `<div class="bar-plot" style="--threshold:${((100 / scale) * 100).toFixed(2)}%">
        <div class="threshold-line" aria-hidden="true"><span>Seuil</span></div>
        <div class="bar-tracks">${tracks}</div>
    </div><div class="bar-labels">${labels}</div>`;
    el.bars.setAttribute('aria-label', `Consommation de ${buildings.length} bâtiments en pourcentage de leur seuil de référence (orange : proche du seuil, rouge : au-dessus).`);
}

/**
 * renderDonut() : dessine le graphique circulaire (donut en SVG) de la répartition
 * de la consommation par ressource (électricité, eau, chauffage), avec la légende
 * (valeur + pourcentage) et le total au centre. Anime l'apparition des segments.
 */
function renderDonut() {
    const totals = Object.fromEntries(ORDER.map((key) => [key, 0]));
    state.filtered.forEach((m) => { totals[m.type_ressource] += m.valeur_consommation; });
    const total = ORDER.reduce((sum, key) => sum + totals[key], 0);

    if (!total) {
        el.donut.innerHTML = '<circle class="donut-ring" cx="21" cy="21" r="15.9155"></circle>';
        el.donutTotal.textContent = '--';
        el.donutLabel.textContent = 'aucune donnée';
        el.legend.innerHTML = '<li class="pie-empty">Aucune mesure à répartir.</li>';
        return;
    }

    let cumul = 0;
    const circles = [];
    const legend = [];

    ORDER.forEach((type) => {
        const value = totals[type];
        if (!value) return;

        const pct = (value / total) * 100;
        const dash = `${pct.toFixed(4)} ${(100 - pct).toFixed(4)}`;
        const unit = type === 'eau' ? 'm³' : 'kWh';

        circles.push(`<circle class="donut-seg" cx="21" cy="21" r="15.9155"
            stroke="${RES[type].color}" stroke-dasharray="${dash}"
            stroke-dashoffset="${(-cumul).toFixed(4)}" data-dash="${dash}"></circle>`);
        legend.push(`<li>
            <span class="legend-swatch" style="background:${RES[type].color}"></span>
            <span class="legend-label">${RES[type].label}</span>
            <span class="legend-value">${fmtNum(value)} ${unit} · ${Math.round(pct)} %</span>
        </li>`);
        cumul += pct;
    });

    el.donut.innerHTML = `<circle class="donut-ring" cx="21" cy="21" r="15.9155"></circle>
        <g transform="rotate(-90 21 21)">${circles.join('')}</g>`;
    el.donutTotal.textContent = fmtNum(total);
    el.donutLabel.textContent = 'unités cumulées';
    el.legend.innerHTML = legend.join('');

    if (!reduced()) {
        $$('.donut-seg', el.donut).forEach((segment) => {
            const dash = segment.dataset.dash;
            segment.setAttribute('stroke-dasharray', '0 100');
            requestAnimationFrame(() => requestAnimationFrame(() => segment.setAttribute('stroke-dasharray', dash)));
        });
    }
}

/* ------------------------------------------------------------------ */
/* Tableau triable                                                    */
/* ------------------------------------------------------------------ */


// Description des colonnes triables : pour chacune, comment lire la valeur à comparer et si c'est un nombre (tri numérique) ou un texte (tri alphabétique).
const COLUMNS = {
    nom_batiment: { value: (m) => m.nom_batiment },
    type_ressource: { value: (m) => RES[m.type_ressource].label },
    date_heure: { value: (m) => m.date_heure },
    valeur_consommation: { value: (m) => m.valeur_consommation, number: true },
    seuil_reference: { value: (m) => m.seuil_reference, number: true },
    alerte: { value: (m) => NIVEAUX.indexOf(m.niveau), number: true }
};


/**
 * tableRow(m, index) : construit le code HTML d'une ligne du tableau pour une mesure (bâtiment, ressource, date, valeur, seuil, statut). La ligne est rouge si la mesure est en alerte.
 */
function tableRow(m, index) {
    const unit = unitFor(m);
    const statut = m.niveau === 'alerte'
        ? { row: 'alert-row', cls: 'status-alert', label: 'ALERTE' }
        : m.niveau === 'proche_seuil'
            ? { row: 'near-row', cls: 'status-proche', label: 'Proche seuil' }
            : { row: '', cls: 'status-ok', label: 'Normal' };
    return `<tr class="${statut.row}" style="--i:${index}">
        <td data-label="Bâtiment"><span class="cell">${esc(m.nom_batiment)}</span></td>
        <td data-label="Ressource"><span class="cell"><span class="badge badge-${m.type_ressource}">${RES[m.type_ressource].label}</span></span></td>
        <td data-label="Date / heure"><span class="cell">${dateShort(m.date_heure)}</span></td>
        <td data-label="Valeur" class="num"><span class="cell">${fmtNum(m.valeur_consommation)} <small>${unit}</small></span></td>
        <td data-label="Seuil" class="num"><span class="cell">${fmtNum(m.seuil_reference)} <small>${unit}</small></span></td>
        <td data-label="Statut"><span class="cell"><span class="status ${statut.cls}">${statut.label}</span></span></td>
    </tr>`;
}


/**
 * renderTable() : trie les mesures filtrées selon la colonne choisie, génère les lignes du tableau (ou un message vide), affiche le compteur "X mesure(s) sur Y"
 * et met à jour les flèches de tri (↑ ↓ ↕) dans l'en-tête.
 */
function renderTable() {
    const column = COLUMNS[state.sort.key] || COLUMNS.date_heure;
    const rows = state.filtered.slice().sort((a, b) => {
        const va = column.value(a);
        const vb = column.value(b);
        const compare = column.number ? va - vb : collator.compare(String(va), String(vb));
        return (compare || collator.compare(a.nom_batiment, b.nom_batiment)) * state.sort.dir;
    });

    el.body.innerHTML = rows.length
        ? rows.map(tableRow).join('')
        : `<tr class="empty-row"><td colspan="6">${empty(
            state.mesures.length ? 'Aucune mesure ne correspond aux filtres' : 'Aucune donnée chargée',
            state.mesures.length ? 'Modifiez la recherche, le bâtiment, la ressource ou la période.' : 'Chargez data.json pour afficher les mesures.'
        )}</td></tr>`;

    el.count.textContent = `${fmtInt(state.filtered.length)} mesure(s) affichée(s) sur ${fmtInt(state.mesures.length)}`;

    $$('th[data-key]').forEach((th) => {
        const active = th.dataset.key === state.sort.key;
        th.setAttribute('aria-sort', active ? (state.sort.dir === 1 ? 'ascending' : 'descending') : 'none');
        th.querySelector('.sort-ind').textContent = active ? (state.sort.dir === 1 ? '↑' : '↓') : '↕';
    });
}


/**
 * toggleSort(key) : appelée au clic sur un en-tête de colonne.
 * Si on reclique sur la même colonne, inverse le sens du tri ; sinon trie sur la nouvelle colonne
 * (décroissant pour la date, croissant pour les autres), puis réaffiche le tableau.
 */

function toggleSort(key) {
    if (!COLUMNS[key]) return;
    if (state.sort.key === key) {
        state.sort.dir *= -1;
    } else {
        state.sort = { key, dir: key === 'date_heure' ? -1 : 1 };
    }
    renderTable();
}

/* ------------------------------------------------------------------ */
/* Interface : statut, thème, notifications, état vide                */
/* ------------------------------------------------------------------ */


/**
 * refresh() : fonction centrale de mise à jour
 */
function refresh() {
    state.filtered = filterMesures();
    renderIndicators();
    renderBars();
    renderDonut();
    renderTable();
}

/**
 * setStatus(text, kind) : affiche le message d'état du chargement des données
 * (ex : "120 mesures chargées") avec une classe de couleur selon le type (ok, loading, error).
 */
function setStatus(text, kind) {
    el.status.textContent = text;
    el.status.className = 'data-status' + (kind ? ' is-' + kind : '');
}


/**
 * updateFooter() : met à jour le pied de page avec la source des données
 * et la date de génération (si elle existe dans les métadonnées du JSON).
 */
function updateFooter() {
    const parts = [];
    if (state.source) parts.push('Source : ' + state.source);
    if (state.meta.genere_le) parts.push('données générées le ' + dateLong(state.meta.genere_le));
    el.footer.textContent = parts.join(' · ');
}


/**
 * emptyUI(title, message) : met toute l'interface en "état vide" (aucune donnée) :
 * statistiques à "--", graphiques et tableau remplacés par un message explicatif.
 * Utilisée quand le chargement de data.json échoue.
 */
function emptyUI(title, message) {
    [el.total, el.moyenne, el.max, el.min, el.alerts, el.near].forEach((node) => {
        node.textContent = '--';
        node.dataset.value = '0';
    });
    el.alertCard.classList.remove('has-alerts');
    el.alertsLabel.textContent = 'alertes';
    el.nearCard.classList.remove('has-near');
    el.nearLabel.textContent = 'mesures';

    const html = empty(title, message);
    el.bars.innerHTML = html;
    el.donut.innerHTML = '<circle class="donut-ring" cx="21" cy="21" r="15.9155"></circle>';
    el.donutTotal.textContent = '--';
    el.donutLabel.textContent = 'aucune donnée';
    el.legend.innerHTML = `<li class="pie-empty">${esc(message)}</li>`;
    el.body.innerHTML = `<tr class="empty-row"><td colspan="6">${html}</td></tr>`;
    el.count.textContent = '0 mesure';
    el.footer.textContent = '';
}

/**
 * setTheme(theme, save) : applique le thème à l'interface (light ou dark)
 */
function setTheme(theme, save) {
    document.documentElement.dataset.theme = theme;
    const dark = theme === 'dark';
    el.theme.setAttribute('aria-pressed', String(dark));
    el.themeLabel.textContent = dark ? 'Mode clair' : 'Mode sombre';
    if (save) {
        try { localStorage.setItem(THEME_KEY, theme); } catch (error) { /* stockage indisponible */ }
    }
}

/**
 * initTheme() : choisit le thème au démarrage : celui sauvegardé par l'utilisateur,
 * sinon celui préféré par son système (clair ou sombre).
 */
function initTheme() {
    let saved = null;
    try { saved = localStorage.getItem(THEME_KEY); } catch (error) { saved = null; }
    const preferred = matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light';
    setTheme(saved === 'dark' || saved === 'light' ? saved : preferred, false);
}


/**
 * toast(message, type, duration) : affiche une petite notification temporaire
 * (info, success, warning, danger) qui disparaît après "duration" ms
 * ou quand on clique sur la croix.
 */
function toast(message, type = 'info', duration = 4500) {
    const node = document.createElement('div');
    node.className = `toast toast-${type}`;
    node.setAttribute('role', type === 'danger' ? 'alert' : 'status');
    node.innerHTML = `<span class="toast-dot" aria-hidden="true"></span>
        <p>${esc(message)}</p>
        <button type="button" class="toast-close" aria-label="Fermer la notification">×</button>`;

    const close = () => {
        node.classList.add('is-leaving');
        setTimeout(() => node.remove(), 280);
    };

    node.querySelector('.toast-close').addEventListener('click', close);
    el.toasts.appendChild(node);
    if (duration > 0) setTimeout(close, duration);
}

/* ------------------------------------------------------------------ */
/* Assistant conversationnel local   ( chatbot )                                 */
/* ------------------------------------------------------------------ */


/*
 * Le chatbot ne fait appel à aucune IA externe : il analyse la question de l'utilisateur
 * (mots-clés, nom de bâtiment, ressource) et calcule la réponse à partir des données chargées.
 */
const chatSum = (rows, key) => rows.reduce((sum, m) => sum + m[key], 0);
// Retourne l'unité d'une ressource : m³ pour l'eau, kWh pour les autres.
const chatUnit = (type) => (type === 'eau' ? 'm³' : 'kWh');

/**
 * chatFindBuilding(raw) : cherche si la question mentionne un bâtiment connu.
 * Compare sans accents, soit le nom complet, soit un mot du nom (4 lettres minimum).
 * Retourne le nom du bâtiment trouvé, ou null.
 */
function chatFindBuilding(raw) {
    const q = norm(raw);
    const names = [...new Set(state.mesures.map((m) => m.nom_batiment))]
        .sort((a, b) => b.length - a.length);
    return names.find((name) => {
        const n = norm(name);
        if (q.includes(n)) return true;
        return n.split(/[^a-z0-9]+/).some((part) => part.length >= 4 && q.includes(part));
    }) || null;
}

/**
 * chatFindResource(raw) : détecte si la question parle d'électricité, d'eau ou de chauffage
 * (avec plusieurs variantes de mots). Retourne la clé de la ressource, ou null.
 */
function chatFindResource(raw) {
    const q = norm(raw);
    if (/\b(electricite|electrique|electriques|elec)\b/.test(q)) return 'electricite';
    if (/\b(eau|eaux)\b/.test(q)) return 'eau';
    if (/\b(chauffage|chauffages|chauffer|chauff)\b/.test(q)) return 'chauffage';
    return null;
}

/**
 * chatListAlerts(rows, title) : construit la réponse listant les dépassements de seuil
 * parmi les mesures données (8 lignes maximum, puis "… et X autre(s)").
 * Si aucune alerte, indique qu'il n'y a aucun dépassement.
 */
function chatListAlerts(rows, title) {
    const alerts = rows.filter((m) => m.alerte);
    if (!alerts.length) return `${title} : aucun dépassement de seuil détecté.`;
    const lines = alerts.slice(0, 8).map((m) =>
        `• ${dateShort(m.date_heure)} — ${m.nom_batiment} (${RES[m.type_ressource].label}) : ${fmtNum(m.valeur_consommation)} ${unitFor(m)} pour un seuil de ${fmtNum(m.seuil_reference)}`);
    const extra = alerts.length > 8 ? `\n… et ${fmtInt(alerts.length - 8)} autre(s).` : '';
    return `${title} : ${fmtInt(alerts.length)} dépassement(s) de seuil.\n${lines.join('\n')}${extra}`;
}


/**
 * chatBuildingReport(name) : rapport complet d'un bâtiment : consommation cumulée,
 * seuil cumulé et pourcentage consommé, nombre de dépassements,
 * puis le détail par ressource (électricité, eau, chauffage).
 */
function chatBuildingReport(name) {
    const rows = state.mesures.filter((m) => m.nom_batiment === name);
    const total = chatSum(rows, 'valeur_consommation');
    const seuil = chatSum(rows, 'seuil_reference');
    const ratio = seuil ? Math.round((total / seuil) * 100) : 0;
    const alerts = rows.filter((m) => m.alerte).length;
    const details = ORDER.map((type) => {
        const subset = rows.filter((m) => m.type_ressource === type);
        return subset.length
            ? `• ${RES[type].label} : ${fmtNum(chatSum(subset, 'valeur_consommation'))} ${chatUnit(type)}`
            : null;
    }).filter(Boolean);
    return [
        `${name} : ${fmtNum(total)} unités cumulées sur ${fmtInt(rows.length)} mesure(s).`,
        `Seuil cumulé : ${fmtNum(seuil)} (${ratio} % consommé).`,
        alerts ? `${fmtInt(alerts)} mesure(s) au-dessus du seuil.` : 'Aucun dépassement de seuil.',
        '',
        ...details
    ].join('\n');
}


/**
 * chatBuildingResource(name, type) : réponse pour un bâtiment ET une ressource précis
 * (ex : "eau de la Bibliothèque") : total, % du seuil et nombre de dépassements.
 */
function chatBuildingResource(name, type) {
    const rows = state.mesures.filter((m) => m.nom_batiment === name && m.type_ressource === type);
    if (!rows.length) return `${name} n’a aucune mesure de ${RES[type].label.toLowerCase()} dans data.json.`;
    const total = chatSum(rows, 'valeur_consommation');
    const seuil = chatSum(rows, 'seuil_reference');
    const ratio = seuil ? Math.round((total / seuil) * 100) : 0;
    const alerts = rows.filter((m) => m.alerte).length;
    return `${name} — ${RES[type].label} : ${fmtNum(total)} ${chatUnit(type)} cumulés sur ${fmtInt(rows.length)} mesure(s), soit ${ratio} % du seuil (${fmtNum(seuil)} ${chatUnit(type)}). ${alerts ? fmtInt(alerts) + ' dépassement(s).' : 'Aucun dépassement.'}`;
}


/**
 * chatResourceReport(type) : rapport sur une ressource pour tout le campus :
 * total cumulé, part dans la consommation totale, nombre de mesures et de dépassements.
 */
function chatResourceReport(type) {
    const rows = state.mesures.filter((m) => m.type_ressource === type);
    if (!rows.length) return `Aucune mesure de ${RES[type].label.toLowerCase()} dans data.json.`;
    const total = chatSum(rows, 'valeur_consommation');
    const all = chatSum(state.mesures, 'valeur_consommation');
    const pct = all ? Math.round((total / all) * 100) : 0;
    const alerts = rows.filter((m) => m.alerte).length;
    return `${RES[type].label} : ${fmtNum(total)} ${chatUnit(type)} cumulés (${pct} % du total), ${fmtInt(rows.length)} mesure(s), ${alerts ? fmtInt(alerts) + ' au-dessus du seuil.' : 'aucun dépassement.'}`;
}


/**
 * chatRepartition() : répond à "répartition" : liste chaque ressource avec son total
 * et son pourcentage dans la consommation cumulée du campus.
 */
function chatRepartition() {
    const all = chatSum(state.mesures, 'valeur_consommation');
    const lines = ORDER.map((type) => {
        const rows = state.mesures.filter((m) => m.type_ressource === type);
        if (!rows.length) return null;
        const total = chatSum(rows, 'valeur_consommation');
        const pct = all ? Math.round((total / all) * 100) : 0;
        return `• ${RES[type].label} : ${fmtNum(total)} ${chatUnit(type)} (${pct} %)`;
    }).filter(Boolean);
    return `Répartition de la consommation cumulée :\n${lines.join('\n')}`;
}

/**
 * chatRank(ascending) : trouve le bâtiment le moins consommateur (ascending = true)
 * ou le plus consommateur (ascending = false) en triant les bâtiments par consommation totale.
 */
function chatRank(ascending) {
    const buildings = computeStats(state.mesures).buildings.slice()
        .sort((a, b) => a.total - b.total);
    const b = ascending ? buildings[0] : buildings[buildings.length - 1];
    const ratio = b.seuil ? Math.round((b.total / b.seuil) * 100) : 0;
    return ascending
        ? `Le bâtiment le moins consommateur est ${b.nom} avec ${fmtNum(b.total)} unités cumulées (${ratio} % de son seuil cumulé).`
        : `Le bâtiment le plus consommateur est ${b.nom} avec ${fmtNum(b.total)} unités cumulées (${ratio} % de son seuil cumulé).`;
}

/**
 * chatExtremes() : retrouve la mesure la plus élevée et la plus faible de tout le jeu de données
 * (avec bâtiment, ressource et date) grâce à reduce().
 */
function chatExtremes() {
    const max = state.mesures.reduce((a, b) => (b.valeur_consommation > a.valeur_consommation ? b : a));
    const min = state.mesures.reduce((a, b) => (b.valeur_consommation < a.valeur_consommation ? b : a));
    return `Maximum : ${fmtNum(max.valeur_consommation)} ${unitFor(max)} (${max.nom_batiment}, ${RES[max.type_ressource].label}, ${dateShort(max.date_heure)}).\nMinimum : ${fmtNum(min.valeur_consommation)} ${unitFor(min)} (${min.nom_batiment}, ${RES[min.type_ressource].label}, ${dateShort(min.date_heure)}).`;
}

/**
 * chatPeriod() : indique la période couverte par les données
 * (première et dernière date) et le nombre total de mesures.
 */
function chatPeriod() {
    const dates = state.mesures.map((m) => m.date_heure.slice(0, 10)).sort();
    return `Les données couvrent la période du ${dateLong(dates[0])} au ${dateLong(dates[dates.length - 1])}, soit ${fmtInt(state.mesures.length)} mesures.`;
}


/**
 * chatPeriod() : indique la période couverte par les données
 * (première et dernière date) et le nombre total de mesures.
 */
function chatHelp() {
    return [
        'Voici ce que je peux faire :',
        '• consommation totale, moyenne, maximum / minimum',
        '• détail d’un bâtiment (ex. « Bibliothèque »)',
        '• détail d’une ressource (électricité, eau, chauffage)',
        '• liste des alertes et bâtiments concernés',
        '• bâtiment le plus / le moins consommateur',
        '• répartition par ressource et période couverte',
        '',
        'Je réponds uniquement à partir des données de data.json.'
    ].join('\n');
}


/**
 * chatAnswer(raw) : "cerveau" du chatbot. Analyse la question et choisit la bonne réponse :
 * 1) vérifie que les données sont chargées ;
 * 2) détecte un bâtiment, une ressource ou une intention d'alerte (priorité à ces cas) ;
 * 3) sinon cherche des mots-clés (répartition, plus/moins consommateur, max/min, moyenne,
 *    total, liste des bâtiments, période, aide, politesse) ;
 * 4) sinon renvoie un message "je n'ai pas compris" avec des exemples de questions.
 */
function chatAnswer(raw) {
    const q = norm(raw);
    const has = (...words) => words.some((word) => q.includes(word));

    if (!state.mesures.length)
        return 'Les données ne sont pas encore chargées. Cliquez sur « Charger data.json » ou rechargez la page, puis reposez votre question.';

    const building = chatFindBuilding(raw);
    const resource = chatFindResource(raw);
    const alertIntent = has('alerte', 'anomalie', 'depassement', 'depasse', 'depassent', 'probleme', 'au-dessus');

    if (building) {
        if (alertIntent) return chatListAlerts(state.mesures.filter((m) => m.nom_batiment === building), building);
        if (resource) return chatBuildingResource(building, resource);
        return chatBuildingReport(building);
    }

    if (resource) {
        if (alertIntent) return chatListAlerts(state.mesures.filter((m) => m.type_ressource === resource), `Alertes ${RES[resource].label.toLowerCase()}`);
        return chatResourceReport(resource);
    }

    if (alertIntent) return chatListAlerts(state.mesures, 'Alertes');

    if (has('repartition', 'proportion', 'pourcentage')) return chatRepartition();
    if (has('plus gros', 'plus grand', 'plus consommateur', 'plus gourmand', 'plus energivore', 'pire')) return chatRank(false);
    if (has('moins consommateur', 'moins gourmand', 'moins energivore', 'plus economique', 'meilleur') || (has('moins') && !has('plus'))) return chatRank(true);
    if (has('maximum', 'minimum', 'maxi', 'mini')) return chatExtremes();
    if (has('moyenne', 'moyen')) {
        const stats = computeStats(state.mesures);
        return `La moyenne est de ${fmtNum(stats.moyenne)} unités cumulées par bâtiment (${fmtInt(stats.buildings.length)} bâtiments).`;
    }
    if (has('total', 'combien consomme', 'consommation globale')) {
        const stats = computeStats(state.mesures);
        return `La consommation totale est de ${fmtNum(stats.total)} unités cumulées sur ${fmtInt(state.mesures.length)} mesures. ${stats.alerts ? fmtInt(stats.alerts) + ' mesure(s) dépassent leur seuil.' : 'Aucune mesure ne dépasse son seuil.'}`;
    }
    if (has('batiment', 'batiments', 'liste')) {
        const names = [...new Set(state.mesures.map((m) => m.nom_batiment))].sort(collator.compare);
        return `Les ${fmtInt(names.length)} bâtiments suivis sont : ${names.join(', ')}.`;
    }
    if (has('periode', 'date', 'dates', 'quand')) return chatPeriod();
    if (has('aide', 'help', 'peux', 'capacite', 'commandes', 'fonctionne')) return chatHelp();
    if (has('merci', 'super', 'parfait', 'nickel')) return 'Avec plaisir ! Une autre question sur la consommation du campus ?';
    if (has('bonjour', 'salut', 'hello', 'bonsoir', 'coucou', 'hey')) return 'Bonjour ! Je suis l’assistant EcoCampus. Posez-moi une question sur la consommation, les bâtiments, les ressources ou les alertes.';

    return 'Je n’ai pas bien compris. Je peux vous renseigner sur la consommation totale, un bâtiment, une ressource ou les alertes. Essayez « Quelles sont les alertes ? » ou « Consommation de la Bibliothèque ».';
}


/**
 * initChatbot() : initialise l'interface du chatbot (bouton flottant + fenêtre de discussion).
 * Elle gère : l'ouverture/fermeture (bouton, bouton ×, touche Échap), l'envoi d'un message
 * (formulaire), les questions suggérées (boutons), l'affichage des messages,
 * l'animation "en train d'écrire" et le message de bienvenue (affiché une seule fois).
 */
function initChatbot() {
    const toggle = $('#chat-toggle');
    const panel = $('#chat-panel');
    if (!toggle || !panel) return;

    const closeBtn = $('#chat-close');
    const log = $('#chat-log');
    const form = $('#chat-form');
    const input = $('#chat-input');
    const suggestions = $('#chat-suggestions');
    let greeted = false;

    const scroll = () => { log.scrollTop = log.scrollHeight; };

    const addMessage = (text, who) => {
        const node = document.createElement('div');
        node.className = `chat-msg ${who}`;
        node.textContent = text;
        log.appendChild(node);
        scroll();
    };

    // Affiche les 3 points "en train d'écrire", puis après un court délai
    // remplace-les par la réponse calculée par chatAnswer().
    const respond = (text) => {
        const typing = document.createElement('div');
        typing.className = 'chat-msg bot chat-typing';
        typing.innerHTML = '<span></span><span></span><span></span>';
        log.appendChild(typing);
        scroll();
        setTimeout(() => {
            typing.remove();
            addMessage(chatAnswer(text), 'bot');
        }, reduced() ? 0 : 420);
    };
    // Ouvre la fenêtre du chatbot, met à jour l'accessibilité, affiche le message de bienvenue
    // (une seule fois) et place le curseur dans le champ de saisie.
    const open = () => {
        panel.hidden = false;
        toggle.setAttribute('aria-expanded', 'true');
        toggle.setAttribute('aria-label', 'Fermer l’assistant EcoCampus');
        if (!greeted) {
            greeted = true;
            addMessage('Bonjour ! Je suis l’assistant EcoCampus. Posez-moi une question sur la consommation, les bâtiments, les ressources ou les alertes — je réponds à partir de data.json.', 'bot');
        }
        setTimeout(() => input.focus(), 40);
    };

    const close = () => {
        panel.hidden = true;
        toggle.setAttribute('aria-expanded', 'false');
        toggle.setAttribute('aria-label', 'Ouvrir l’assistant EcoCampus');
        toggle.focus();
    };

    toggle.addEventListener('click', () => {
        if (panel.hidden) open(); else close();
    });
    closeBtn.addEventListener('click', close);
    panel.addEventListener('keydown', (event) => {
        if (event.key === 'Escape') close();
    });

    form.addEventListener('submit', (event) => {
        event.preventDefault();
        const text = input.value.trim();
        if (!text) return;
        addMessage(text, 'user');
        input.value = '';
        respond(text);
    });

    suggestions.addEventListener('click', (event) => {
        const button = event.target.closest('button[data-q]');
        if (!button) return;
        addMessage(button.dataset.q, 'user');
        respond(button.dataset.q);
    });
}

/* ------------------------------------------------------------------ */
/* Liaison des événements et démarrage                                */
/* ------------------------------------------------------------------ */


/**
 * fillBuildings() : remplit la liste déroulante "Bâtiment" avec les noms
 * uniques des bâtiments présents dans les données, triés par ordre alphabétique.
 */
function fillBuildings() {
    const names = [...new Set(state.mesures.map((m) => m.nom_batiment))].sort(collator.compare);
    el.building.innerHTML = '<option value="">Tous</option>' +
        names.map((name) => `<option>${esc(name)}</option>`).join('');
}


/**
 * clearFilters() : remet à zéro tous les champs de filtre
 * (recherche, bâtiment, ressource, dates).
 */
function clearFilters() {
    el.search.value = '';
    el.building.value = '';
    el.resource.value = '';
    el.dateStart.value = '';
    el.dateEnd.value = '';
}


/**
 * setDateBounds() : limite les sélecteurs de date entre la première et la dernière
 * date présentes dans les données (attributs min et max).
 */
function setDateBounds() {
    const dates = state.mesures.map((m) => m.date_heure.slice(0, 10)).sort();
    if (!dates.length) return;
    [el.dateStart, el.dateEnd].forEach((input) => {
        input.min = dates[0];
        input.max = dates[dates.length - 1];
    });
}


/**
 * bindEvents() : branche tous les écouteurs d'événements :
 * recherche (avec debounce), filtres, bouton de réinitialisation, menu des filtres (mobile),
 * changement de thème, chargement manuel d'un fichier, tri par clic/clavier sur les colonnes
 * et glisser-déposer d'un fichier JSON sur la page.
 */
function bindEvents() {
    const debouncedRefresh = debounce(refresh, 120);

    el.search.addEventListener('input', debouncedRefresh);
    [el.building, el.resource, el.dateStart, el.dateEnd].forEach((input) => {
        input.addEventListener('change', refresh);
    });

    el.reset.addEventListener('click', () => {
        clearFilters();
        state.sort = { key: 'date_heure', dir: -1 };
        refresh();
        el.search.focus();
    });

    el.filtersToggle.addEventListener('click', () => {
        const open = el.filters.classList.toggle('is-open');
        el.filtersToggle.setAttribute('aria-expanded', String(open));
    });

    el.theme.addEventListener('click', () => {
        setTheme(document.documentElement.dataset.theme === 'dark' ? 'light' : 'dark', true);
    });

    el.load.addEventListener('click', () => el.file.click());
    el.file.addEventListener('change', () => readFile(el.file.files[0]));

    $$('th[data-key]').forEach((th) => {
        const sort = () => toggleSort(th.dataset.key);
        th.addEventListener('click', sort);
        th.addEventListener('keydown', (event) => {
            if (event.key === 'Enter' || event.key === ' ') {
                event.preventDefault();
                sort();
            }
        });
    });

    window.addEventListener('dragover', (event) => event.preventDefault());
    window.addEventListener('drop', (event) => {
        event.preventDefault();
        readFile(event.dataTransfer && event.dataTransfer.files[0]);
    });
}


/**
 * init() : point d'entrée de l'application. Récupère les éléments HTML,
 * branche les événements, initialise le chatbot et le thème, puis charge data.json.
 */
function init() {
    cacheElements();
    bindEvents();
    initChatbot();
    initTheme();
    loadData();
}
// Lance init() dès que la page est prête (immédiatement si elle est déjà chargée).
if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', init);
} else {
    init();
}
