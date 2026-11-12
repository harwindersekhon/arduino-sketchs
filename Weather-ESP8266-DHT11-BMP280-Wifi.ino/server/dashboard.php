<?php
/*
 * Weather Station dashboard
 * Reads the rows that insert_data.php stores and charts them with Chart.js.
 *
 * Expected table (edit the CONFIG block below if your names differ):
 *
 *   CREATE TABLE weather_data (
 *     id           INT AUTO_INCREMENT PRIMARY KEY,
 *     dht_temp     FLOAT,
 *     humidity     FLOAT,
 *     bmp_temp     FLOAT,
 *     pressure     FLOAT,
 *     reading_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
 *     INDEX (reading_time)
 *   );
 */

/* ---------- CONFIG ---------- */
const DB_HOST  = 'localhost';
const DB_NAME  = 'weather';
const DB_USER  = 'weather_user';
const DB_PASS  = 'change_me';
const DB_TABLE = 'weather_data';
const TIME_COL = 'reading_time';
const OFFLINE_AFTER_MIN = 15;   // station counts as offline after this long without data (ESP uploads every 5 min)

/* ---------- REQUEST ---------- */
$ranges = [6 => '6 h', 24 => '24 h', 168 => '7 days', 720 => '30 days'];
$hours  = (int)($_GET['hours'] ?? 24);
if (!isset($ranges[$hours])) {
    $hours = 24;
}
// Average readings into buckets so long ranges stay around 400 points per line
$bucket = max(300, intdiv($hours * 3600, 400));
$since  = time() - $hours * 3600;

/* ---------- DATA ---------- */
$keys   = ['dht_temp', 'humidity', 'bmp_temp', 'pressure'];
$series = array_fill_keys($keys, []);
$latest = null;
$count  = 0;
$error  = null;

try {
    $pdo = new PDO(
        'mysql:host=' . DB_HOST . ';dbname=' . DB_NAME . ';charset=utf8mb4',
        DB_USER,
        DB_PASS,
        [PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION, PDO::ATTR_DEFAULT_FETCH_MODE => PDO::FETCH_ASSOC]
    );
    $t = '`' . DB_TABLE . '`';
    $c = '`' . TIME_COL . '`';

    $latest = $pdo->query(
        "SELECT dht_temp, humidity, bmp_temp, pressure, UNIX_TIMESTAMP($c) AS ts
         FROM $t ORDER BY $c DESC LIMIT 1"
    )->fetch() ?: null;

    $stmt = $pdo->prepare(
        "SELECT FLOOR(UNIX_TIMESTAMP($c) / :b1) * :b2 AS bucket_ts,
                COUNT(*) AS n,
                AVG(dht_temp) AS dht_temp, AVG(humidity) AS humidity,
                AVG(bmp_temp) AS bmp_temp, AVG(pressure) AS pressure
         FROM $t
         WHERE $c >= FROM_UNIXTIME(:since)
         GROUP BY bucket_ts
         ORDER BY bucket_ts"
    );
    $stmt->bindValue(':b1', $bucket, PDO::PARAM_INT);
    $stmt->bindValue(':b2', $bucket, PDO::PARAM_INT);
    $stmt->bindValue(':since', $since, PDO::PARAM_INT);
    $stmt->execute();

    $prev = null;
    foreach ($stmt as $row) {
        $ts = (int)$row['bucket_ts'];
        // Station was offline: add a null point so the line breaks instead of bridging the gap
        if ($prev !== null && $ts - $prev > 3 * $bucket) {
            foreach ($keys as $k) {
                $series[$k][] = ['x' => ($prev + $bucket) * 1000, 'y' => null];
            }
        }
        foreach ($keys as $k) {
            $series[$k][] = ['x' => $ts * 1000, 'y' => $row[$k] === null ? null : round((float)$row[$k], 2)];
        }
        $count += (int)$row['n'];
        $prev = $ts;
    }
} catch (PDOException $e) {
    error_log('dashboard.php: ' . $e->getMessage());
    $error = 'Could not read weather data from the database. Check the CONFIG block in dashboard.php.';
}

/* ---------- HELPERS ---------- */
function fmt($v, int $dec): string
{
    return $v === null ? '--' : number_format((float)$v, $dec, '.', '');
}

function ago(int $sec): string
{
    if ($sec < 60)    return 'just now';
    if ($sec < 3600)  return intdiv($sec, 60) . ' min ago';
    if ($sec < 86400) return intdiv($sec, 3600) . ' h ago';
    return intdiv($sec, 86400) . ' days ago';
}

$age    = $latest ? time() - (int)$latest['ts'] : null;
$online = $age !== null && $age <= OFFLINE_AFTER_MIN * 60;
?>
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Weather Station</title>
<style>
  :root {
    --bg: #f4f5f7; --card: #ffffff; --text: #1d2330; --muted: #6b7280; --line: #e5e7eb;
    --grid: rgba(0, 0, 0, .07); --ok: #15803d; --bad: #b91c1c;
    --c-temp: #ea580c; --c-bmp: #db2777; --c-hum: #2563eb; --c-pres: #7c3aed;
  }
  @media (prefers-color-scheme: dark) {
    :root {
      --bg: #0f1218; --card: #181c25; --text: #e5e7eb; --muted: #9ca3af; --line: #262b36;
      --grid: rgba(255, 255, 255, .08); --ok: #4ade80; --bad: #f87171;
      --c-temp: #fb923c; --c-bmp: #f472b6; --c-hum: #60a5fa; --c-pres: #a78bfa;
    }
  }
  * { box-sizing: border-box; }
  body {
    margin: 0; padding: 24px 16px; background: var(--bg); color: var(--text);
    font: 15px/1.4 system-ui, -apple-system, "Segoe UI", Roboto, sans-serif;
  }
  main { max-width: 1100px; margin: 0 auto; }
  header { display: flex; flex-wrap: wrap; align-items: center; justify-content: space-between; gap: 12px; margin-bottom: 16px; }
  h1 { font-size: 22px; margin: 0; }
  h2 { font-size: 14px; font-weight: 600; margin: 0 0 8px; color: var(--muted); }
  .status { font-size: 14px; color: var(--muted); }
  .status b { color: var(--bad); }
  .status.ok b { color: var(--ok); }
  .ranges { display: flex; gap: 6px; flex-wrap: wrap; margin-bottom: 16px; }
  .ranges a {
    padding: 5px 12px; border: 1px solid var(--line); border-radius: 999px;
    color: var(--text); text-decoration: none; font-size: 14px; background: var(--card);
  }
  .ranges a.active { background: var(--text); color: var(--bg); border-color: var(--text); }
  .cards { display: grid; grid-template-columns: repeat(auto-fit, minmax(160px, 1fr)); gap: 12px; margin-bottom: 16px; }
  .card { background: var(--card); border: 1px solid var(--line); border-radius: 10px; padding: 14px 16px; }
  .card .value { font-size: 28px; font-weight: 600; font-variant-numeric: tabular-nums; }
  .card .unit { font-size: 15px; color: var(--muted); margin-left: 2px; }
  .charts { display: grid; gap: 12px; }
  .chart-box { position: relative; height: 260px; }
  .error, .empty { background: var(--card); border: 1px solid var(--line); border-radius: 10px; padding: 16px; }
  .error { border-color: var(--bad); color: var(--bad); }
  footer { margin-top: 16px; font-size: 13px; color: var(--muted); }
</style>
</head>
<body>
<main>
  <header>
    <h1>Weather Station</h1>
    <div class="status<?= $online ? ' ok' : '' ?>">
      <b>&#9679; <?= $online ? 'Online' : 'Offline' ?></b>
      <?= $latest ? '&middot; last reading ' . ago($age) : '&middot; no readings yet' ?>
    </div>
  </header>

  <nav class="ranges">
    <?php foreach ($ranges as $h => $label): ?>
      <a href="?hours=<?= $h ?>" class="<?= $h === $hours ? 'active' : '' ?>"><?= $label ?></a>
    <?php endforeach; ?>
  </nav>

  <?php if ($error): ?>
    <p class="error"><?= htmlspecialchars($error) ?></p>
  <?php endif; ?>

  <section class="cards">
    <div class="card"><h2>Temperature (DHT11)</h2>
      <span class="value"><?= fmt($latest['dht_temp'] ?? null, 1) ?></span><span class="unit">&deg;C</span></div>
    <div class="card"><h2>Humidity</h2>
      <span class="value"><?= fmt($latest['humidity'] ?? null, 0) ?></span><span class="unit">%</span></div>
    <div class="card"><h2>Pressure</h2>
      <span class="value"><?= fmt($latest['pressure'] ?? null, 1) ?></span><span class="unit">hPa</span></div>
    <div class="card"><h2>Temperature (BMP280)</h2>
      <span class="value"><?= fmt($latest['bmp_temp'] ?? null, 1) ?></span><span class="unit">&deg;C</span></div>
  </section>

  <?php if (!$error && $count === 0): ?>
    <p class="empty">No readings in the last <?= $ranges[$hours] ?>.</p>
  <?php elseif (!$error): ?>
    <section class="charts">
      <div class="card"><h2>Temperature (&deg;C)</h2><div class="chart-box"><canvas id="tempChart"></canvas></div></div>
      <div class="card"><h2>Humidity (%)</h2><div class="chart-box"><canvas id="humChart"></canvas></div></div>
      <div class="card"><h2>Pressure (hPa)</h2><div class="chart-box"><canvas id="presChart"></canvas></div></div>
    </section>
  <?php endif; ?>

  <footer>
    <?= $count ?> readings in the last <?= $ranges[$hours] ?><?= $bucket > 300 ? ', averaged per ' . round($bucket / 60) . ' min' : '' ?>
    &middot; page refreshes every 5 min
  </footer>
</main>

<?php if (!$error && $count > 0): ?>
<script src="https://cdn.jsdelivr.net/npm/chart.js@4.4.1/dist/chart.umd.min.js"></script>
<script src="https://cdn.jsdelivr.net/npm/chartjs-adapter-date-fns@3.0.0/dist/chartjs-adapter-date-fns.bundle.min.js"></script>
<script>
  const series = <?= json_encode($series, JSON_HEX_TAG | JSON_PRESERVE_ZERO_FRACTION) ?>;

  const css = getComputedStyle(document.documentElement);
  const color = name => css.getPropertyValue(name).trim();
  Chart.defaults.color = color('--muted');
  Chart.defaults.borderColor = color('--grid');
  Chart.defaults.font.family = getComputedStyle(document.body).fontFamily;

  function lineChart(canvasId, lines, unit, decimals) {
    new Chart(document.getElementById(canvasId), {
      type: 'line',
      data: {
        datasets: lines.map(l => ({
          label: l.label,
          data: series[l.key],
          borderColor: color(l.color),
          backgroundColor: color(l.color),
          borderWidth: 2,
          pointRadius: 0,
          pointHoverRadius: 4,
          tension: 0.3,
          spanGaps: false,
        })),
      },
      options: {
        responsive: true,
        maintainAspectRatio: false,
        animation: false,
        interaction: { mode: 'index', intersect: false },
        scales: {
          x: { type: 'time', time: { tooltipFormat: 'd MMM, HH:mm' }, grid: { display: false } },
          y: { ticks: { callback: v => v + unit } },
        },
        plugins: {
          legend: { display: lines.length > 1, labels: { boxWidth: 12, boxHeight: 2 } },
          tooltip: {
            callbacks: {
              label: c => c.parsed.y === null ? null : `${c.dataset.label}: ${c.parsed.y.toFixed(decimals)}${unit}`,
            },
          },
        },
      },
    });
  }

  lineChart('tempChart', [
    { key: 'dht_temp', label: 'DHT11', color: '--c-temp' },
    { key: 'bmp_temp', label: 'BMP280', color: '--c-bmp' },
  ], ' °C', 1);
  lineChart('humChart', [{ key: 'humidity', label: 'Humidity', color: '--c-hum' }], ' %', 0);
  lineChart('presChart', [{ key: 'pressure', label: 'Pressure', color: '--c-pres' }], ' hPa', 1);
</script>
<?php endif; ?>
<script>
  setTimeout(() => location.reload(), 5 * 60 * 1000);  // matches the ESP upload interval
</script>
</body>
</html>
