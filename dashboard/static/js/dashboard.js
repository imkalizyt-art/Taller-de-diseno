const body = document.body;
const threshold = Number(body.dataset.threshold || 35);
const refreshMs = Number(body.dataset.refreshSeconds || 10) * 1000;

const elements = {
    currentHumidity: document.getElementById("currentHumidity"),
    lastUpdate: document.getElementById("lastUpdate"),
    stateBadge: document.getElementById("stateBadge"),
    recommendation: document.getElementById("recommendation"),
    adcValue: document.getElementById("adcValue"),
    averageValue: document.getElementById("averageValue"),
    minimumValue: document.getElementById("minimumValue"),
    maximumValue: document.getElementById("maximumValue"),
    sampleCount: document.getElementById("sampleCount"),
    connectionPill: document.getElementById("connectionPill"),
    connectionText: document.getElementById("connectionText"),
    deviceName: document.getElementById("deviceName"),
    errorMessage: document.getElementById("errorMessage"),
};

const chartContext = document.getElementById("humidityChart");

const chart = new Chart(chartContext, {
    type: "line",
    data: {
        labels: [],
        datasets: [
            {
                label: "Humedad (%)",
                data: [],
                borderColor: "#2d7a4c",
                backgroundColor: "rgba(45, 122, 76, 0.12)",
                fill: true,
                tension: 0.28,
                pointRadius: 2,
                pointHoverRadius: 5,
                borderWidth: 2.5,
            },
            {
                label: `Umbral (${threshold}%)`,
                data: [],
                borderColor: "#c2413a",
                borderDash: [7, 6],
                pointRadius: 0,
                borderWidth: 1.7,
            },
        ],
    },
    options: {
        responsive: true,
        maintainAspectRatio: false,
        interaction: {
            mode: "index",
            intersect: false,
        },
        scales: {
            y: {
                suggestedMin: 0,
                suggestedMax: 100,
                ticks: {
                    callback: (value) => `${value}%`,
                },
                grid: {
                    color: "rgba(24, 53, 42, 0.07)",
                },
            },
            x: {
                grid: {
                    display: false,
                },
                ticks: {
                    maxTicksLimit: 8,
                },
            },
        },
        plugins: {
            legend: {
                position: "bottom",
                labels: {
                    usePointStyle: true,
                    boxWidth: 8,
                    padding: 20,
                },
            },
            tooltip: {
                callbacks: {
                    label: (context) => {
                        if (context.datasetIndex === 0) {
                            return ` Humedad: ${context.parsed.y.toFixed(1)}%`;
                        }
                        return ` Umbral: ${threshold}%`;
                    },
                },
            },
        },
    },
});

function formatNumber(value, decimals = 1) {
    if (value === null || value === undefined) return "--";
    return Number(value).toFixed(decimals);
}

function formatDate(isoDate) {
    if (!isoDate) return "sin datos";

    return new Intl.DateTimeFormat("es-CL", {
        dateStyle: "short",
        timeStyle: "medium",
    }).format(new Date(isoDate));
}

function setConnectionStatus(isOnline) {
    elements.connectionPill.classList.remove("online", "offline");
    elements.connectionPill.classList.add(isOnline ? "online" : "offline");
    elements.connectionText.textContent = isOnline
        ? "Dispositivo conectado"
        : "Sin datos recientes";
}

function updateSummary(data) {
    const current = data.actual;
    const stats = data.estadisticas;

    elements.currentHumidity.textContent = current
        ? formatNumber(current.humedad)
        : "--";
    elements.adcValue.textContent = current ? current.adc : "--";
    elements.lastUpdate.textContent = current
        ? formatDate(current.created_at)
        : "sin datos";
    elements.deviceName.textContent = current
        ? current.dispositivo
        : "ESP8266";

    elements.averageValue.textContent = formatNumber(stats.promedio);
    elements.minimumValue.textContent = formatNumber(stats.minimo);
    elements.maximumValue.textContent = formatNumber(stats.maximo);
    elements.sampleCount.textContent =
        `${stats.cantidad} ${stats.cantidad === 1 ? "medición" : "mediciones"}`;

    elements.stateBadge.textContent = data.estado;
    elements.recommendation.textContent = data.recomendacion;
    elements.stateBadge.className = "state-badge neutral";

    if (current) {
        elements.stateBadge.className = current.suelo_seco
            ? "state-badge dry"
            : "state-badge good";
    }

    setConnectionStatus(data.dispositivo_online);
}

function updateChart(measurements) {
    const labels = measurements.map((item) =>
        new Intl.DateTimeFormat("es-CL", {
            hour: "2-digit",
            minute: "2-digit",
        }).format(new Date(item.created_at))
    );

    const humidityValues = measurements.map((item) => item.humedad);

    chart.data.labels = labels;
    chart.data.datasets[0].data = humidityValues;
    chart.data.datasets[1].data = labels.map(() => threshold);
    chart.update("none");
}

async function fetchJson(url) {
    const response = await fetch(url, {
        headers: {
            Accept: "application/json",
        },
        cache: "no-store",
    });

    const data = await response.json();

    if (!response.ok) {
        throw new Error(data.error || "No se pudo consultar el servidor.");
    }

    return data;
}

async function refreshDashboard() {
    try {
        const [summary, measurements] = await Promise.all([
            fetchJson("/api/resumen"),
            fetchJson("/api/mediciones?limit=120"),
        ]);

        updateSummary(summary);
        updateChart(measurements);
        elements.errorMessage.textContent = "";
    } catch (error) {
        elements.errorMessage.textContent =
            `No fue posible actualizar el dashboard: ${error.message}`;
        setConnectionStatus(false);
    }
}

refreshDashboard();
setInterval(refreshDashboard, refreshMs);
