async function loadData() {
    try {
        const response = await fetch("/api/data");
        const devices = await response.json();

        const container = document.getElementById("tables");
        container.innerHTML = "";

        devices.forEach((device) => {

            const dados = device.Dados || [];

            // Ordena por canal (garante ordem correta)
            dados.sort((a, b) => a.I - b.I);

            // Divide em dois blocos
            const grupos = [
                dados.filter(d => d.I >= 1 && d.I <= 6),
                dados.filter(d => d.I >= 7 && d.I <= 12)
            ];

            grupos.forEach((grupo, indexGrupo) => {

                const card = document.createElement("div");
                card.className = "card";

                const inicio = indexGrupo === 0 ? 1 : 7;
                const fim = indexGrupo === 0 ? 6 : 12;

                card.style.cursor = "pointer";
                card.addEventListener("click", () => {
                    abrirGrafico(device.ID, inicio, fim);
                });

                const title = document.createElement("div");
                title.className = "card-title";
                title.textContent = `DESCESP ${device.ID} - ${String(inicio).padStart(2,"0")} à ${String(fim).padStart(2,"0")}`;
                card.appendChild(title);

                const table = document.createElement("table");
                table.innerHTML = `
                    <thead>
                        <tr>
                            <th>Canal</th>
                            <th>Tensão (V)</th>
                            <th>Temperatura (°C)</th>
                            <th>Umidade (%)</th>
                            <th>Data/Hora</th>
                        </tr>
                    </thead>
                    <tbody></tbody>
                `;

                const tbody = table.querySelector("tbody");

                // adiciona os canais existentes
                grupo.forEach((d) => {
                    const dataFormatada = new Date(d.timestamp * 1000).toLocaleString("pt-BR");
                    const row = document.createElement("tr");

                    // --- CONFIGURAÇÃO DE LIMITES ---
                    const tensao = parseFloat(d.Tensao);
                    const limiteCritico = 15.0; // Abaixo disso fica Vermelho
                    const limiteFading = 16.0;  // Abaixo disso a linha esmaece
                    
                    let classeCor = "";
                    
                    if (!isNaN(tensao)) {
                        // Define a cor do texto
                        classeCor = tensao < limiteCritico ? "text-critical" : "text-normal";
                        
                        // Define se a linha esmaece (valor muito baixo ou crítico)
                        if (tensao < limiteFading) {
                            row.classList.add("faded-row");
                        }
                    }
                    // ------------------------------
                    
                    // Estilo para indicar que a linha é clicável
                    row.style.cursor = "cell"; 

                    row.innerHTML = `
                        <td>${d.I}</td>
                        <td class="${classeCor}">${d.Tensao ?? "—"}</td>
                        <td>${d.Temp ?? "—"}</td>
                        <td>${d.Umi ?? "—"}</td>
                        <td>${dataFormatada}</td>
                    `;

                    // EVENTO DE CLIQUE NA LINHA
                    row.addEventListener("click", (e) => {
                        e.stopPropagation(); // IMPEDE de abrir o modal do card (pai)
                        abrirGraficoCanal(device.ID, d.I);
                    });

                    tbody.appendChild(row);
                });

                // completa canais ausentes até 6 linhas
                for (let i = grupo.length; i < 6; i++) {

                    const canal = inicio + i;

                    const row = document.createElement("tr");
                    row.innerHTML = `
                        <td>${canal}</td>
                        <td>—</td>
                        <td>—</td>
                        <td>—</td>
                        <td>—</td>
                    `;
                    tbody.appendChild(row);
                }

                card.appendChild(table);
                container.appendChild(card);
            });
        });

    } catch (err) {
        console.error("Erro ao carregar dados:", err);
    }
}

// Variáveis globais para controlar o modal
const modal = document.getElementById("chartModal");
const canvas = document.getElementById("chartCanvas");
const ctx = canvas.getContext("2d");
let chartInstance = null;

const channelModal = document.getElementById("channelModal");
const channelCanvas = document.getElementById("channelCanvas");
let channelChartInstance = null;

// Lógica para fechar qualquer modal ao clicar fora
window.addEventListener("click", (e) => {
    if (e.target.classList.contains('modal-overlay') || e.target.classList.contains('close-button')) {
        const modalToClose = e.target.closest('.modal-overlay') || e.target;
        modalToClose.classList.remove("active");
        
        // Destruir instâncias após a animação
        setTimeout(() => {
            if (modalToClose.id === "chartModal" && chartInstance) {
                chartInstance.destroy(); chartInstance = null;
            }
            if (modalToClose.id === "channelModal" && channelChartInstance) {
                channelChartInstance.destroy(); channelChartInstance = null;
            }
        }, 400);
    }
});

async function abrirGraficoCanal(deviceId, channelIndex) {
    try {
        const response = await fetch(`/api/history/channel/${deviceId}/${channelIndex}`);
        const data = await response.json();

        document.getElementById("channelModalTitle").textContent = `DESCESP ${deviceId} - Canal ${String(channelIndex).padStart(2, "0")}`;
        channelModal.classList.add("active");

        if (channelChartInstance) channelChartInstance.destroy();

        channelChartInstance = new Chart(channelCanvas.getContext("2d"), {
            type: "line",
            data: {
                labels: data.timestamps.map(t => new Date(t * 1000).toLocaleTimeString("pt-BR")),
                datasets: [
                    {
                        label: "Tensão (V)",
                        data: data.tensao,
                        borderColor: "#4f46e5",
                        backgroundColor: "#4f46e5",
                        yAxisID: 'y',
                    },
                    {
                        label: "Temp (°C)",
                        data: data.temp,
                        borderColor: "#ff9f43",
                        backgroundColor: "#ff9f43",
                        yAxisID: 'y1',
                    },
                    {
                        label: "Umi (%)",
                        data: data.umi,
                        borderColor: "#0fc592",
                        backgroundColor: "#0fc592",
                        yAxisID: 'y2',
                    }
                ]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                interaction: {
                    mode: 'index',     // mostra todos os datasets no mesmo índice (timestamp)
                    intersect: false,   // não precisa estar exatamente sobre o ponto
                    axis: 'x'
                },
                plugins: {
                    legend: { 
                        position: "top",
                        labels: { color: '#fff' }
                    },
                    tooltip: {
                        enabled: true,
                        mode: 'index',    // Mostra todos os datasets do índice
                        intersect: false, // Fundamental para não precisar "mirar" no ponto
                        position: 'nearest', // Opcional: melhora a posição da caixa de texto
                        backgroundColor: 'rgba(15, 24, 44, 0.9)', // Fundo do tooltip combinando com seu tema
                        titleColor: '#fff',
                        bodyColor: '#fff',
                        borderColor: '#32425f',
                        borderWidth: 1
                    }
                },
                scales: {
                    y: { // Tensão
                        type: 'linear', display: true, position: 'left',
                        ticks: { color: '#4f46e5' },
                        grid: { color: 'rgba(255,255,255,0.1)' }
                    },
                    y1: { // Temperatura
                        type: 'linear', display: true, position: 'right',
                        ticks: { color: '#ff9f43' },
                        grid: { drawOnChartArea: false } // evita bagunça de linhas
                    },
                    y2: { // Umidade
                        type: 'linear', display: true, position: 'right',
                        offset: true,
                        ticks: { color: '#0fc592' },
                        grid: { drawOnChartArea: false }
                    },
                    x: { ticks: { color: '#fff' } }
                }
            }
        });
    } catch (err) {
        console.error("Erro ao carregar canal:", err);
    }
}

async function abrirGrafico(deviceId, inicio, fim) {
    try {
        const response = await fetch(`/api/history/${deviceId}/${inicio}/${fim}`);
        const data = await response.json();

        // Mostra o modal
        modal.classList.add("active");

        // Se já houver um gráfico, destrói antes de criar o novo
        if (chartInstance) chartInstance.destroy();

        const datasets = Object.keys(data.canais).map(canal => {
            // Pega a cor fixa; se o canal não estiver no objeto, usa um cinza padrão
            const corFixa = coresCanais[canal] || "#94a3b8";
            return {
                label: "Canal " + canal,
                data: data.canais[canal],
                borderWidth: 2,
                fill: false,
                borderColor: corFixa,
                backgroundColor: corFixa, // Importante para a bolinha da legenda
                tension: 0.1 // Deixa a linha levemente suavizada
            };
        });

        chartInstance = new Chart(ctx, {
            type: "line",
            data: {
                labels: data.timestamps.map(t =>
                    new Date(t * 1000).toLocaleTimeString("pt-BR")
                ),
                datasets: datasets
            },
            options: {
                responsive: true, // Mudei para true para melhor ajuste
                maintainAspectRatio: false,
                interaction: {
                    mode: 'index',     // mostra todos os datasets no mesmo índice (timestamp)
                    intersect: false,   // não precisa estar exatamente sobre o ponto
                    axis: 'x'
                },
                plugins: {
                    legend: { 
                        position: "top",
                        labels: { color: '#fff' }
                    },
                    tooltip: {
                        enabled: true,
                        mode: 'index',    // Mostra todos os datasets do índice
                        intersect: false, // Fundamental para não precisar "mirar" no ponto
                        position: 'nearest', // Opcional: melhora a posição da caixa de texto
                        backgroundColor: 'rgba(15, 24, 44, 0.9)', // Fundo do tooltip combinando com seu tema
                        titleColor: '#fff',
                        bodyColor: '#fff',
                        borderColor: '#32425f',
                        borderWidth: 1,
                        callbacks: {
                            // Opcional: formatação para garantir que a unidade apareça no tooltip
                            label: function(context) {
                                return `${context.dataset.label}: ${context.parsed.y} V`;
                            }
                        }
                    }
                },
                scales: {
                    y: {
                        ticks: { color: '#fff' },
                        title: { display: true, text: "Tensão (V)", color: '#fff' }
                    },
                    x: {ticks: { color: '#fff' }}
                }
            }
        });
    } catch (err) {
        console.error("Erro ao carregar histórico:", err);
    }
}

// Objeto de cores fixas
const coresCanais = {
    "1": "#4f46e5", // Indigo
    "2": "#0fc592", // Verde
    "3": "#f59e0b", // Amarelo/Laranja
    "4": "#ec4899", // Rosa
    "5": "#4c3f69", // Roxo
    "6": "#06b6d4", // Ciano
    "7": "#ef4444", // Vermelho
    "8": "#10b981", // Esmeralda
    "9": "#f97316", // Laranja Forte
    "10": "#3b82f6", // Azul
    "11": "#f1dc63", // Violeta
    "12": "#a855f7"  // Púrpura
};

// primeira carga
loadData();

// auto refresh
setInterval(loadData, 5000);