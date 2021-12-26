#include "vhci_pnp_relations.h"
#include "trace.h"
#include "vhci_pnp_relations.tmh"

#include "vhci_irp.h"
#include "vhci.h"

static PAGEABLE void relations_deref_devobj(PDEVICE_RELATIONS relations, ULONG idx)
{
	PAGED_CODE();

	ObDereferenceObject(relations->Objects[idx]);
	if (idx < relations->Count - 1)
		RtlCopyMemory(relations->Objects + idx, relations->Objects + idx + 1, sizeof(PDEVICE_OBJECT) * (relations->Count - 1 - idx));
}

static PAGEABLE BOOLEAN relations_has_devobj(PDEVICE_RELATIONS relations, PDEVICE_OBJECT devobj, BOOLEAN deref)
{
	PAGED_CODE();

	ULONG	i;

	for (i = 0; i < relations->Count; i++) {
		if (relations->Objects[i] == devobj) {
			if (deref)
				relations_deref_devobj(relations, i);
			return TRUE;
		}
	}
	return FALSE;
}

static PAGEABLE NTSTATUS get_bus_relations_1_child(pvdev_t vdev, PDEVICE_RELATIONS *pdev_relations)
{
	PAGED_CODE();

	BOOLEAN	child_exist = TRUE;
	PDEVICE_RELATIONS	relations = *pdev_relations, relations_new;
	PDEVICE_OBJECT	devobj_cpdo;
	ULONG	size;

	if (vdev->child_pdo == nullptr || vdev->child_pdo->DevicePnPState == Deleted)
		child_exist = FALSE;

	if (relations == nullptr) {
		relations = (PDEVICE_RELATIONS)ExAllocatePoolWithTag(PagedPool, sizeof(DEVICE_RELATIONS), USBIP_VHCI_POOL_TAG);
		if (relations == nullptr) {
			TraceError(FLAG_GENERAL, "no relations will be reported: out of memory");
			return STATUS_INSUFFICIENT_RESOURCES;
		}
		relations->Count = 0;
	}
	if (!child_exist) {
		*pdev_relations = relations;
		return STATUS_SUCCESS;
	}

	devobj_cpdo = vdev->child_pdo->Self;
	if (relations->Count == 0) {
		*pdev_relations = relations;
		relations->Count = 1;
		relations->Objects[0] = devobj_cpdo;
		ObReferenceObject(devobj_cpdo);
		return STATUS_SUCCESS;
	}
	if (relations_has_devobj(relations, devobj_cpdo, !child_exist)) {
		*pdev_relations = relations;
		return STATUS_SUCCESS;
	}

	// Need to allocate a new relations structure and add vhub to it
	size = sizeof(DEVICE_RELATIONS) + relations->Count * sizeof(PDEVICE_OBJECT);
	relations_new = (PDEVICE_RELATIONS)ExAllocatePoolWithTag(PagedPool, size, USBIP_VHCI_POOL_TAG);
	if (relations_new == nullptr) {
		TraceError(FLAG_GENERAL, "old relations will be used: out of memory");
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	RtlCopyMemory(relations_new->Objects, relations->Objects, sizeof(PDEVICE_OBJECT) * relations->Count);
	relations_new->Count = relations->Count + 1;
	relations_new->Objects[relations->Count] = devobj_cpdo;
	ObReferenceObject(devobj_cpdo);

	ExFreePool(relations);
	*pdev_relations = relations_new;

	return STATUS_SUCCESS;
}

static pvpdo_dev_t
find_managed_vpdo(pvhub_dev_t vhub, PDEVICE_OBJECT devobj)
{
	PLIST_ENTRY	entry;

	for (entry = vhub->head_vpdo.Flink; entry != &vhub->head_vpdo; entry = entry->Flink) {
		pvpdo_dev_t	vpdo = CONTAINING_RECORD(entry, vpdo_dev_t, Link);
		if (vpdo->common.Self == devobj) {
			return vpdo;
		}
	}
	return nullptr;
}

static BOOLEAN
is_in_dev_relations(PDEVICE_OBJECT devobjs[], ULONG n_counts, pvpdo_dev_t vpdo)
{
	ULONG	i;

	for (i = 0; i < n_counts; i++) {
		if (vpdo->common.Self == devobjs[i]) {
			return TRUE;
		}
	}
	return FALSE;
}

static PAGEABLE NTSTATUS get_bus_relations_vhub(pvhub_dev_t vhub, PDEVICE_RELATIONS *pdev_relations)
{
	PAGED_CODE();

	PDEVICE_RELATIONS	relations_old = *pdev_relations, relations;
	ULONG			length, n_olds = 0, n_news = 0;
	PLIST_ENTRY		entry;
	ULONG	i;

	ExAcquireFastMutex(&vhub->Mutex);

	if (relations_old)
		n_olds = relations_old->Count;

	// Need to allocate a new relations structure and add our vpdos to it
	length = sizeof(DEVICE_RELATIONS) + (vhub->n_vpdos_plugged + n_olds - 1) * sizeof(PDEVICE_OBJECT);

	relations = (PDEVICE_RELATIONS)ExAllocatePoolWithTag(PagedPool, length, USBIP_VHCI_POOL_TAG);
	if (relations == nullptr) {
		TraceError(FLAG_GENERAL, "failed to allocate a new relation: out of memory");

		ExReleaseFastMutex(&vhub->Mutex);
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	for (i = 0; i < n_olds; i++) {
		pvpdo_dev_t	vpdo;
		PDEVICE_OBJECT	devobj = relations_old->Objects[i];
		vpdo = find_managed_vpdo(vhub, devobj);
		if (vpdo == nullptr || vpdo->plugged) {
			relations->Objects[n_news] = devobj;
			n_news++;
		}
		else {
			ObDereferenceObject(devobj);
		}
	}

	for (entry = vhub->head_vpdo.Flink; entry != &vhub->head_vpdo; entry = entry->Flink) {
		pvpdo_dev_t	vpdo = CONTAINING_RECORD(entry, vpdo_dev_t, Link);

		if (is_in_dev_relations(relations->Objects, n_news, vpdo))
			continue;
		if (vpdo->plugged) {
			relations->Objects[n_news] = vpdo->common.Self;
			n_news++;
			ObReferenceObject(vpdo->common.Self);
		}
	}

	relations->Count = n_news;

	TraceInfo(FLAG_GENERAL, "vhub vpdos: total:%u,plugged:%u: bus relations: old:%u,new:%u", vhub->n_vpdos, vhub->n_vpdos_plugged, n_olds, n_news);

	if (relations_old)
		ExFreePool(relations_old);

	ExReleaseFastMutex(&vhub->Mutex);

	*pdev_relations = relations;
	return STATUS_SUCCESS;
}

static PAGEABLE PDEVICE_RELATIONS get_self_dev_relation(pvdev_t vdev)
{
	PAGED_CODE();

	PDEVICE_RELATIONS	dev_relations;

	dev_relations = (PDEVICE_RELATIONS)ExAllocatePoolWithTag(PagedPool, sizeof(DEVICE_RELATIONS), USBIP_VHCI_POOL_TAG);
	if (dev_relations == nullptr)
		return nullptr;

	// There is only one vpdo in the structure
	// for this relation type. The PnP Manager removes
	// the reference to the vpdo when the driver or application
	// un-registers for notification on the device.
	dev_relations->Count = 1;
	dev_relations->Objects[0] = vdev->Self;
	ObReferenceObject(vdev->Self);

	return dev_relations;
}

static PAGEABLE NTSTATUS get_bus_relations(pvdev_t vdev, PDEVICE_RELATIONS *pdev_relations)
{
	PAGED_CODE();

	switch (vdev->type) {
	case VDEV_ROOT:
	case VDEV_VHCI:
		return get_bus_relations_1_child(vdev, pdev_relations);
	case VDEV_VHUB:
		return get_bus_relations_vhub((pvhub_dev_t)vdev, pdev_relations);
	default:
		return STATUS_NOT_SUPPORTED;
	}
}

static PAGEABLE NTSTATUS get_target_relation(pvdev_t vdev, PDEVICE_RELATIONS *pdev_relations)
{
	PAGED_CODE();

	if (vdev->type != VDEV_VPDO)
		return STATUS_NOT_SUPPORTED;

	if (*pdev_relations) {
		// Only vpdo can handle this request. Somebody above
		// is not playing by rule.
		NT_ASSERT(!"Someone above is handling TagerDeviceRelation");
	}
	*pdev_relations = get_self_dev_relation(vdev);
	return STATUS_SUCCESS;
}

PAGEABLE NTSTATUS pnp_query_device_relations(pvdev_t vdev, PIRP irp)
{
	PAGED_CODE();

	IO_STACK_LOCATION *irpstack = IoGetCurrentIrpStackLocation(irp);
	TraceInfo(FLAG_GENERAL, "%!vdev_type_t!: %!DEVICE_RELATION_TYPE!", vdev->type, irpstack->Parameters.QueryDeviceRelations.Type);

	DEVICE_RELATIONS *dev_relations = (DEVICE_RELATIONS*)irp->IoStatus.Information;
	NTSTATUS status = STATUS_SUCCESS;

	switch (irpstack->Parameters.QueryDeviceRelations.Type) {
	case TargetDeviceRelation:
		status = get_target_relation(vdev, &dev_relations);
		break;
	case BusRelations:
		status = get_bus_relations(vdev, &dev_relations);
		break;
	case RemovalRelations:
	case EjectionRelations:
		if (is_fdo(vdev->type)) {
			return irp_pass_down(vdev->devobj_lower, irp);
		}
		status = STATUS_SUCCESS;
		break;
	default:
		TraceInfo(FLAG_GENERAL, "skip: %!DEVICE_RELATION_TYPE!", irpstack->Parameters.QueryDeviceRelations.Type);
		status = irp->IoStatus.Status;
	}

	if (NT_SUCCESS(status)) {
		irp->IoStatus.Information = (ULONG_PTR)dev_relations;
	}

	return irp_done(irp, status);
}