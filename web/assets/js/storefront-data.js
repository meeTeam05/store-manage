window.storefrontData = {
  brandName: "Store Manage",
  accounts: [
    {
      accountId: "account-001",
      username: "client001",
      passwordHash: "hash:123456",
      role: "Customer",
      customerId: "customer-001"
    },
    {
      accountId: "account-002",
      username: "staff001",
      passwordHash: "hash:staff123",
      role: "Staff",
      employeeId: "employee-001"
    },
    {
      accountId: "account-003",
      username: "admin001",
      passwordHash: "hash:admin123",
      role: "Admin",
      employeeId: "employee-002"
    }
  ],
  customers: [
    {
      customerId: "customer-001",
      accountId: "account-001",
      fullName: "Nguyen Van A",
      phone: "0900000000",
      city: "Ho Chi Minh City",
      address: {
        recipientName: "Nguyen Van A",
        phone: "0900000000",
        line1: "12 Nguyen Hue",
        line2: "",
        ward: "Ben Nghe",
        district: "District 1",
        city: "Ho Chi Minh City",
        country: "Vietnam"
      },
      wishlist: []
    }
  ],
  employees: [
    {
      employeeId: "employee-001",
      accountId: "account-002",
      fullName: "Store Staff",
      position: "Sales Staff"
    },
    {
      employeeId: "employee-002",
      accountId: "account-003",
      fullName: "System Admin",
      position: "Administrator"
    }
  ],
  products: [
    {
      productId: "product-001",
      name: "Structured Coat",
      category: "Outerwear",
      description: "Long-line black coat with quiet shoulder structure, fluid drape, and a disciplined silhouette for day-to-evening wear.",
      priceMinor: 4900000,
      images: [
        "outerwear_1.jpg",
        "outerwear_2.jpg"
      ],
      variants: [
        { variantId: "variant-001-black-s", size: "S", color: "Black", sku: "COAT-BLK-S", stockQuantity: 10 },
        { variantId: "variant-001-black-m", size: "M", color: "Black", sku: "COAT-BLK-M", stockQuantity: 12 },
        { variantId: "variant-001-black-l", size: "L", color: "Black", sku: "COAT-BLK-L", stockQuantity: 8 },
        { variantId: "variant-001-ivory-s", size: "S", color: "Ivory", sku: "COAT-IVR-S", stockQuantity: 9 },
        { variantId: "variant-001-ivory-m", size: "M", color: "Ivory", sku: "COAT-IVR-M", stockQuantity: 9 }
      ]
    },
    {
      productId: "product-002",
      name: "Ivory Column Dress",
      category: "Dresses",
      description: "Fluid ivory column dress with a draped cowl neck, open back, and floor-skimming hem for elevated evening wear.",
      priceMinor: 5600000,
      images: [
        "dresses_1.jpg",
        "dresses_2.jpg"
      ],
      variants: [
        { variantId: "variant-002-ivory-s", size: "S", color: "Ivory", sku: "DRESS-IVR-S", stockQuantity: 5 },
        { variantId: "variant-002-ivory-m", size: "M", color: "Ivory", sku: "DRESS-IVR-M", stockQuantity: 4 },
        { variantId: "variant-002-ivory-l", size: "L", color: "Ivory", sku: "DRESS-IVR-L", stockQuantity: 3 },
        { variantId: "variant-002-beige-s", size: "S", color: "Beige", sku: "DRESS-BEG-S", stockQuantity: 6 },
        { variantId: "variant-002-beige-m", size: "M", color: "Beige", sku: "DRESS-BEG-M", stockQuantity: 5 }
      ]
    },
    {
      productId: "product-003",
      name: "Tailored Blazer",
      category: "Outerwear",
      description: "Classic double-breasted tailored blazer in charcoal wool, features structured shoulders and a modern relaxed cut.",
      priceMinor: 3800000,
      images: [
        "tops_1.jpg",
        "tops_2.jpg"
      ],
      variants: [
        { variantId: "variant-003-charcoal-s", size: "S", color: "Charcoal", sku: "BLZR-CHA-S", stockQuantity: 7 },
        { variantId: "variant-003-charcoal-m", size: "M", color: "Charcoal", sku: "BLZR-CHA-M", stockQuantity: 8 },
        { variantId: "variant-003-charcoal-l", size: "L", color: "Charcoal", sku: "BLZR-CHA-L", stockQuantity: 6 },
        { variantId: "variant-003-black-s", size: "S", color: "Black", sku: "BLZR-BLK-S", stockQuantity: 5 },
        { variantId: "variant-003-black-m", size: "M", color: "Black", sku: "BLZR-BLK-M", stockQuantity: 6 }
      ]
    }
  ],
  voucher: {
    code: "WELCOME10",
    rate: 0.10,
    maxDiscountMinor: 800000,
    minOrderMinor: 1000000
  }
};
