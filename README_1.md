# 💧 AC Water to Aquarium — Turning AC Condensate into Safe Aquarium Water

> Turning AC condensate water (usually wasted down the drain) into safe aquarium water for fish, through a simple process of filtering, aeration, and remineralization.

![Status](https://img.shields.io/badge/status-ongoing-yellow)
![License](https://img.shields.io/badge/license-MIT-blue)
![Category](https://img.shields.io/badge/category-DIY%20%2F%20Aquaculture-green)

---

## 📖 Background

AC condensate water forms from water vapor in the air condensing on the evaporator coil — not from refrigerant — so it's naturally similar to distilled water: low in limescale and heavy minerals. This water is usually just drained away, even though it has real potential to be processed into good-quality aquarium water, especially for soft-water fish (tetras, rasboras, discus).

The challenge: AC water can be contaminated by dust, mold/algae spores from the drain pipe, trace metal residue from the evaporator, and it's too mineral-poor to be used directly by fish.

## 🎯 Project Goals

- Make use of AC drain water that would otherwise go to waste
- Design a simple, low-cost treatment method so AC water is safe for aquarium use
- Test the treated water's suitability using water quality parameters and indicator fish

## 🧪 AC Water Characteristics vs. Aquarium Requirements

| Parameter | Raw AC Water | Ideal Aquarium Range |
|---|---|---|
| TDS | Very low (< 20 ppm) | 50–150 ppm (species-dependent) |
| pH | Tends to be acidic (5.5–6.5) | 6.5–7.5 |
| Minerals (Ca, Mg) | Almost none | Needed for fish osmoregulation |
| Contaminants | Dust, mold/algae spores, trace metals | Must be contaminant-free |
| Dissolved oxygen | Low right when it drips out | Needs aeration |

## 🛠️ Tools & Materials

| Category | Item |
|---|---|
| Initial collection | Clean, covered bucket/jerry can |
| Physical filter | Filter cloth / cotton filter |
| Chemical filter | Activated carbon |
| Additional media (optional) | Zeolite |
| Stabilization | Aerator / air pump |
| Remineralization | Aquarium salt / mineral booster (GH+) |
| Measuring tools | TDS meter, pH meter/test kit, thermometer |
| Safety testing | Hardy indicator fish (betta/molly) |

## 🔄 Process Flow

```
Water drips from the AC unit
        │
        ▼
1. Collection (clean, covered container)
        │
        ▼
2. Physical filtering (cloth/cotton)
        │
        ▼
3. Activated carbon filtration
        │
        ▼
4. Aeration for 24 hours
        │
        ▼
5. Parameter testing (TDS, pH, temperature)
        │
        ▼
6. Remineralization (aquarium salt)
        │
        ▼
7. Indicator fish trial (3–7 days)
        │
        ▼
8. Routine monitoring (weekly)
```

## ✅ Success Criteria

- pH stable within 6.5–7.5
- TDS within the 50–150 ppm range
- Indicator fish show no signs of stress during the trial period

## 📁 Suggested Repo Structure

```
ac-water-to-aquarium/
├── README.md
├── docs/
│   └── AC_Water_to_Aquarium_Report.docx
├── data/
│   └── measurement-log.csv        # daily/weekly TDS, pH, temperature records
├── images/
│   └── (setup photos, process, results)
└── LICENSE
```

## 📊 Measurement Log (example)

| Date | TDS (ppm) | pH | Temp (°C) | Notes |
|---|---|---|---|---|
| 2026-07-29 | 15 | 6.0 | 27 | Raw AC water, before treatment |
| 2026-07-30 | 80 | 7.0 | 27 | After remineralization |
| 2026-08-06 | 85 | 7.1 | 27 | Indicator fish healthy |

## ⚠️ Important Notes

- Never use 100% raw AC water without remineralization — water that's too pure can cause osmotic stress in fish.
- Clean the AC unit and drain pipe regularly.
- For beginners, mixing AC water with mature aquarium water (50:50 ratio) is a more practical approach.

## 🚀 Roadmap

- [ ] Add an automatic filtration system
- [ ] Integrate TDS/pH sensors for real-time monitoring
- [ ] Document before/after photos
- [ ] Test on several different fish species

## 📄 License

This project is licensed under [MIT](LICENSE) — free to use, modify, and share.

## 🙋 Contributing

Pull requests and feedback are welcome! If you try this project at home, feel free to share your measurement results via an issue or PR to the `data/` folder.
